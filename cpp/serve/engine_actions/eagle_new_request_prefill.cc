/*!
 *  Copyright (c) 2023 by Contributors
 * \file serve/engine_actions/eagle_new_request_prefill.cc
 */

#include <tvm/runtime/nvtx.h>

#include "../config.h"
#include "../model.h"
#include "../sampler/sampler.h"
#include "action.h"
#include "action_commons.h"
#include "batch_prefill_base.h"

namespace mlc {
namespace llm {
namespace serve {

/*!
 * \brief The action that prefills requests in the `waiting_queue` of
 * the engine state.
 */
class EagleNewRequestPrefillActionObj : public BatchPrefillBaseActionObj {
 public:
  explicit EagleNewRequestPrefillActionObj(Array<Model> models, LogitProcessor logit_processor,
                                           Sampler sampler,
                                           std::vector<ModelWorkspace> model_workspaces,
                                           DraftTokenWorkspaceManager draft_token_workspace_manager,
                                           EngineConfig engine_config,
                                           Optional<EventTraceRecorder> trace_recorder)
      : BatchPrefillBaseActionObj(std::move(models), std::move(engine_config),
                                  std::move(trace_recorder)),
        logit_processor_(std::move(logit_processor)),
        sampler_(std::move(sampler)),
        model_workspaces_(std::move(model_workspaces)),
        draft_token_workspace_manager_(std::move(draft_token_workspace_manager)) {}

  Array<Request> Step(EngineState estate) final {
    // - Find the requests in `waiting_queue` that can prefill in this step.
    std::vector<PrefillInput> prefill_inputs;
    {
      NVTXScopedRange nvtx_scope("NewRequestPrefill getting requests");
      prefill_inputs = GetRequestStateEntriesToPrefill(estate);
      if (prefill_inputs.empty()) {
        return {};
      }
    }

    int num_rsentries = prefill_inputs.size();
    auto tstart = std::chrono::high_resolution_clock::now();

    // - Update status of request states from pending to alive.
    Array<String> request_ids;
    std::vector<RequestState> rstates_of_entries;
    std::vector<RequestStateStatus> status_before_prefill;
    UpdateRequestToAlive(prefill_inputs, estate, &request_ids, &rstates_of_entries,
                         &status_before_prefill);

    // - Get embedding and run prefill for each model.
    std::vector<int> prefill_lengths;
    prefill_lengths.resize(/*size=*/num_rsentries, /*value=*/-1);
    ObjectRef hidden_states_for_input{nullptr};
    ObjectRef hidden_states_for_sample{nullptr};
    NDArray logits_for_sample{nullptr};
    // A map used to record the entry and child_idx pair needed to fork sequence.
    // The base model (id 0) should record all the pairs and all the small models
    // fork sequences according to this map.
    std::unordered_map<int, std::unordered_set<int>> fork_rsentry_child_map;
    for (int model_id = 0; model_id < static_cast<int>(models_.size()); ++model_id) {
      std::vector<int64_t> request_internal_ids;
      request_internal_ids.reserve(num_rsentries);
      ObjectRef embeddings = model_workspaces_[model_id].embeddings;
      int cum_prefill_length = 0;
      bool single_input =
          num_rsentries == 1 && prefill_inputs[0].rsentry->mstates[model_id]->inputs.size() == 1;
      for (int i = 0; i < num_rsentries; ++i) {
        const RequestStateEntry& rsentry = prefill_inputs[i].rsentry;
        RequestModelState mstate = rsentry->mstates[model_id];
        auto [input_data, input_length] =
            ChunkPrefillInputData(mstate, prefill_inputs[i].max_prefill_length);
        if (prefill_lengths[i] == -1) {
          prefill_lengths[i] = input_length;
        } else {
          ICHECK_EQ(prefill_lengths[i], input_length);
        }

        ICHECK(mstate->draft_output_tokens.empty());
        ICHECK(mstate->draft_token_slots.empty());
        if (status_before_prefill[i] == RequestStateStatus::kPending) {
          // Add the sequence to the model, or fork the sequence from its parent.
          if (rsentry->parent_idx == -1) {
            models_[model_id]->AddNewSequence(mstate->internal_id);
          } else {
            models_[model_id]->ForkSequence(
                rstates_of_entries[i]->entries[rsentry->parent_idx]->mstates[model_id]->internal_id,
                mstate->internal_id);
          }
          // Enable sliding window for the sequence if it is not a parent.
          if (rsentry->child_indices.empty()) {
            models_[model_id]->EnableSlidingWindowForSeq(mstate->internal_id);
          }
          // Shift the input tokens by 1 for eagle models.
          if (model_id == 0) {
            for (int j = 1; j < static_cast<int>(models_.size()); ++j) {
              ICHECK(rsentry->mstates[j]->inputs.size());
              TokenData token_data = Downcast<TokenData>(rsentry->mstates[j]->inputs[0]);
              rsentry->mstates[j]->inputs.Set(
                  0, TokenData(
                         IntTuple(token_data->token_ids.begin() + 1, token_data->token_ids.end())));
            }
          }
        }
        request_internal_ids.push_back(mstate->internal_id);
        RECORD_EVENT(trace_recorder_, prefill_inputs[i].rsentry->request->id, "start embedding");
        // Speculative models shift left the input tokens by 1 when base model has committed tokens.
        // Note: for n > 1 cases Eagle doesn't work because parent entry doesn't shift input tokens.
        for (int j = 0; j < static_cast<int>(input_data.size()); ++j) {
          embeddings = input_data[j]->GetEmbedding(
              models_[model_id],
              /*dst=*/!single_input ? &model_workspaces_[model_id].embeddings : nullptr,
              /*offset=*/cum_prefill_length);
          cum_prefill_length += input_data[j]->GetLength();
        }
        RECORD_EVENT(trace_recorder_, rsentry->request->id, "finish embedding");
      }

      RECORD_EVENT(trace_recorder_, request_ids, "start prefill");
      ObjectRef embedding_or_hidden_states{nullptr};
      if (model_id == 0) {
        embedding_or_hidden_states = embeddings;
      } else {
        embedding_or_hidden_states = models_[model_id]->FuseEmbedHidden(
            embeddings, hidden_states_for_input, /*batch_size*/ 1, /*seq_len*/ cum_prefill_length);
      }
      // hidden_states: (b * s, h)
      ObjectRef hidden_states = models_[model_id]->BatchPrefillToLastHidden(
          embedding_or_hidden_states, request_internal_ids, prefill_lengths);
      RECORD_EVENT(trace_recorder_, request_ids, "finish prefill");

      if (model_id == 0) {
        // We only need to sample for model 0 in prefill.
        hidden_states_for_input = hidden_states;
      }

      // Whether to use base model to get logits.
      int sample_model_id = !models_[model_id]->CanGetLogits() ? 0 : model_id;

      std::vector<int> logit_positions;
      {
        // Prepare the logit positions
        logit_positions.reserve(prefill_lengths.size());
        int total_len = 0;
        for (int i = 0; i < prefill_lengths.size(); ++i) {
          total_len += prefill_lengths[i];
          logit_positions.push_back(total_len - 1);
        }
      }
      // hidden_states_for_sample: (b * s, h)
      hidden_states_for_sample = models_[sample_model_id]->GatherHiddenStates(
          hidden_states, logit_positions, &model_workspaces_[model_id].hidden_states);
      // logits_for_sample: (b * s, v)
      logits_for_sample = models_[sample_model_id]->GetLogits(hidden_states_for_sample);
      // - Update logits.
      ICHECK(logits_for_sample.defined());
      Array<GenerationConfig> generation_cfg;
      Array<RequestModelState> mstates_for_logitproc;
      generation_cfg.reserve(num_rsentries);
      mstates_for_logitproc.reserve(num_rsentries);
      for (int i = 0; i < num_rsentries; ++i) {
        generation_cfg.push_back(prefill_inputs[i].rsentry->request->generation_cfg);
        mstates_for_logitproc.push_back(prefill_inputs[i].rsentry->mstates[sample_model_id]);
      }
      logit_processor_->InplaceUpdateLogits(logits_for_sample, generation_cfg,
                                            mstates_for_logitproc, request_ids);

      // - Compute probability distributions.
      NDArray probs_on_device =
          logit_processor_->ComputeProbsFromLogits(logits_for_sample, generation_cfg, request_ids);

      // - Sample tokens.
      //   For prefill_inputs which have children, sample
      //   one token for each rstate that is depending.
      //   Otherwise, sample a token for the current rstate.
      std::vector<int> sample_indices;
      std::vector<RequestStateEntry> rsentries_for_sample;
      std::vector<RandomGenerator*> rngs;
      std::vector<bool> rsentry_activated;
      sample_indices.reserve(num_rsentries);
      rsentries_for_sample.reserve(num_rsentries);
      rngs.reserve(num_rsentries);
      rsentry_activated.reserve(num_rsentries);
      request_ids.clear();
      generation_cfg.clear();
      for (int i = 0; i < num_rsentries; ++i) {
        const RequestStateEntry& rsentry = prefill_inputs[i].rsentry;
        // No sample for rsentries with remaining inputs.
        if (!rsentry->mstates[0]->inputs.empty()) {
          continue;
        }

        int remaining_num_child_to_activate = prefill_inputs[i].num_child_to_activate;
        for (int child_idx : rsentry->child_indices) {
          // Only use base model to judge if we need to add child entries.
          if (rstates_of_entries[i]->entries[child_idx]->status == RequestStateStatus::kPending &&
              (rstates_of_entries[i]->entries[child_idx]->mstates[0]->committed_tokens.empty() ||
               fork_rsentry_child_map[i].count(child_idx))) {
            // If rstates_of_entries[i]->entries[child_idx] has no committed token,
            // the prefill of the current rsentry will unblock
            // rstates_of_entries[i]->entries[child_idx],
            // and thus we want to sample a token for rstates_of_entries[i]->entries[child_idx].
            fork_rsentry_child_map[i].insert(child_idx);
            sample_indices.push_back(i);
            rsentries_for_sample.push_back(rstates_of_entries[i]->entries[child_idx]);
            request_ids.push_back(rsentry->request->id);
            generation_cfg.push_back(rsentry->request->generation_cfg);
            rngs.push_back(&rstates_of_entries[i]->entries[child_idx]->rng);

            // We only fork the first `num_child_to_activate` children.
            // The children not being forked will be forked via later prefills.
            // Usually `num_child_to_activate` is the same as the number of children.
            // But it can be fewer subject to the KV cache max num sequence limit.
            if (remaining_num_child_to_activate == 0) {
              rsentry_activated.push_back(false);
              continue;
            }
            rsentry_activated.push_back(true);
            --remaining_num_child_to_activate;
            if (model_id == 0) {
              ICHECK(rstates_of_entries[i]->entries[child_idx]->status ==
                     RequestStateStatus::kPending);
              rstates_of_entries[i]->entries[child_idx]->status = RequestStateStatus::kAlive;
            }
            int64_t child_internal_id =
                rstates_of_entries[i]->entries[child_idx]->mstates[model_id]->internal_id;
            models_[model_id]->ForkSequence(rsentry->mstates[model_id]->internal_id,
                                            child_internal_id);
            // Enable sliding window for the child sequence if the child is not a parent.
            if (rstates_of_entries[i]->entries[child_idx]->child_indices.empty()) {
              models_[model_id]->EnableSlidingWindowForSeq(child_internal_id);
            }
          }
        }
        if (rsentry->child_indices.empty()) {
          // If rsentry has no child, we sample a token for itself.
          sample_indices.push_back(i);
          rsentries_for_sample.push_back(rsentry);
          request_ids.push_back(rsentry->request->id);
          generation_cfg.push_back(rsentry->request->generation_cfg);
          rngs.push_back(&rsentry->rng);
          rsentry_activated.push_back(true);
        }
      }

      NDArray renormalized_probs = sampler_->BatchRenormalizeProbsByTopP(
          probs_on_device, sample_indices, request_ids, generation_cfg);
      std::vector<SampleResult> sample_results = sampler_->BatchSampleTokensWithProbAfterTopP(
          renormalized_probs, sample_indices, request_ids, generation_cfg, rngs);
      ICHECK_EQ(sample_results.size(), rsentries_for_sample.size());

      // - Update the committed tokens of states.
      // - If a request is first-time prefilled, set the prefill finish time.
      auto tnow = std::chrono::high_resolution_clock::now();
      if (model_id == 0) {
        UpdateRequestStateEntriesWithSampleResults(rsentries_for_sample, rsentry_activated,
                                                   sample_results);
        // Add the sampled token as an input of the eagle models.
        for (int i = 0; i < static_cast<int>(rsentries_for_sample.size()); ++i) {
          for (int mid = 1; mid < static_cast<int>(models_.size()); ++mid) {
            TokenData token_data =
                Downcast<TokenData>(rsentries_for_sample[i]->mstates[mid]->inputs.back());
            std::vector<int32_t> token_ids = {token_data->token_ids.begin(),
                                              token_data->token_ids.end()};
            token_ids.push_back(sample_results[i].sampled_token_id.first);
            int ninputs = static_cast<int>(rsentries_for_sample[i]->mstates[mid]->inputs.size());
            rsentries_for_sample[i]->mstates[mid]->inputs.Set(
                ninputs - 1, TokenData(IntTuple(token_ids.begin(), token_ids.end())));
          }
        }
      } else {
        // - Slice and save hidden_states_for_sample
        draft_token_workspace_manager_->AllocSlots(rsentries_for_sample.size(),
                                                   &draft_token_slots_);
        models_[model_id]->ScatterDraftProbs(renormalized_probs, draft_token_slots_,
                                             &model_workspaces_[0].draft_probs_storage);
        if (engine_config_->spec_draft_length > 1) {
          models_[model_id]->ScatterHiddenStates(hidden_states_for_sample, draft_token_slots_,
                                                 &model_workspaces_[0].draft_hidden_states_storage);
        }
        for (int i = 0; i < static_cast<int>(rsentries_for_sample.size()); ++i) {
          rsentries_for_sample[i]->mstates[model_id]->AddDraftToken(sample_results[i],
                                                                    draft_token_slots_[i]);
          estate->stats.total_draft_length += 1;
        }
      }
    }

    auto tend = std::chrono::high_resolution_clock::now();
    estate->stats.engine_total_prefill_time += static_cast<double>((tend - tstart).count()) / 1e9;

    std::vector<Request> processed_requests =
        RemoveProcessedRequests(prefill_inputs, estate, rstates_of_entries);
    return processed_requests;
  }

 private:
  /*! \brief The logit processor. */
  LogitProcessor logit_processor_;
  /*! \brief The sampler to sample new tokens. */
  Sampler sampler_;
  /*! \brief Workspace of each model. */
  std::vector<ModelWorkspace> model_workspaces_;
  /*! \brief The draft token workspace manager. */
  DraftTokenWorkspaceManager draft_token_workspace_manager_;
  /*! \brief Temporary buffer to store the slots of the current draft tokens */
  std::vector<int> draft_token_slots_;
};

EngineAction EngineAction::EagleNewRequestPrefill(
    Array<Model> models, LogitProcessor logit_processor, Sampler sampler,
    std::vector<ModelWorkspace> model_workspaces,
    DraftTokenWorkspaceManager draft_token_workspace_manager, EngineConfig engine_config,
    Optional<EventTraceRecorder> trace_recorder) {
  return EngineAction(make_object<EagleNewRequestPrefillActionObj>(
      std::move(models), std::move(logit_processor), std::move(sampler),
      std::move(model_workspaces), std::move(draft_token_workspace_manager),
      std::move(engine_config), std::move(trace_recorder)));
}

}  // namespace serve
}  // namespace llm
}  // namespace mlc
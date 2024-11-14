Model Configuration
===================

This section describes model specific configurations for well known models


Compilation process has various stages as described below 

Generate Configuration
----------------------

::

    python -m  mlc_llm gen_config \
            <SOURCE_MODEL> \
            --quantization q4f16_0 \
            --conv-template <MODEL_TEMPLATE> \
            --prefill-chunk-size 256 \
            <ADDITIONAL_OPTIONS> \
            -o <MODEL_OUTPUT_PATH>

Parameter Quantization
----------------------

::

    python -m mlc_llm convert_weight \
           <SOURCE_MODEL>
           --quantization q4f16_0
           -o <MODEL_OUTPUT_PATH>


Model Compilation
-----------------

::

    python -m mlc_llm compile
           <MODEL_OUTPUT_PATH>/mlc-chat-config.json
           --device <DEVICE_CONFIG>
           -o <MODEL_LIB>


Below table describes various configuration options for well known models

.. list-table:: Model specific options
   :widths: 30 70
   :header-rows: 1

   * - Model
     - Options
   * - Llama-2-7b-chat-hf
     - MODEL_TEMPLATE="llama-2"
   * - Meta-Llama-3-8B-Instruct
     - MODEL_TEMPLATE="llama-3"
   * - Qwen-7B-Chat
     - | MODEL_TEMPLATE="chatml"
       | ADDITIONAL_OPTIONS="--model-type qwen --context-window-size 4096" 
   * - Mistral-7B-Instruct-v0.2
     - | MODEL_TEMPLATE="mistral_default"
       | ADDITIONAL_OPTIONS="--sliding-window-size 1024"
   * - gemma-2b-it
     - | MODEL_TEMPLATE="gemma_instruction"
       | ADDITIONAL_OPTIONS="--context-window-size 4096"
   * - phi-2
     - | MODEL_TEMPLATE="phi-2"
       | ADDITIONAL_OPTIONS="--context-window-size 4096"
   * - Phi-3-mini-4k-instruct
     - | MODEL_TEMPLATE="phi-3"
       | ADDITIONAL_OPTIONS="--context-window-size 4096"
   * - llava-1.5-7b-hf
     - | MODEL_TEMPLATE="llava"
       | ADDITIONAL_OPTIONS="--context-window-size 4096"



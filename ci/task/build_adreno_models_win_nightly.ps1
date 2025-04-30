# Set the error handling to stop execution on error
$ErrorActionPreference = "Stop"

$MODEL_ARTIFACTS_PATH = $args[0]

New-Item -ItemType Directory -Path "./dist/libs" -Force

# Function to build the model
function build-model {
    param(
        [string]$model,
        [string]$quantization,
        [string]$template,
        [string]$addl_args
    )

    # Compile the model for Adreno
    $global:LASTEXITCODE = 0
    Invoke-Expression -Command "python -m mlc_llm compile ${MODEL_ARTIFACTS_PATH}/dist/${model}-${quantization}-MLC/mlc-chat-config.json --device windows:adreno_x86 -o ./dist/libs/${model}-${quantization}-adreno.dll"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build the models
build-model Llama-2-7b-chat-hf q4f16_0 llama-2 "--prefill-chunk-size 256"
build-model Meta-Llama-3-8B-Instruct q4f16_0 llama-3 "--prefill-chunk-size 256"
build-model Qwen-7B-Chat q4f16_0 chatml "--model-type qwen --prefill-chunk-size 256 --context-window-size 4096"
build-model Mistral-7B-Instruct-v0.2 q4f16_0 mistral_default "--sliding-window-size 1024 --prefill-chunk-size 256"
build-model gemma-2b-it q4f16_0 gemma_instruction  "--prefill-chunk-size 256 --context-window-size 4096"
build-model phi-2 q4f16_0 phi-2 "--prefill-chunk-size 256 --context-window-size 4096"
build-model Phi-3-mini-4k-instruct q4f16_0 phi-3 "--prefill-chunk-size 256 --context-window-size 4096"
build-model Phi-3.5-mini-instruct q4f16_0 phi-3 "--prefill-chunk-size 256 --context-window-size 4096"
build-model llava-1.5-7b-hf q4f16_0 llava "--prefill-chunk-size 256 --context-window-size 4096"
# build-model Baichuan-7B q4f16_0 chatml "--model-type baichuan --prefill-chunk-size 256 --context-window-size 4096"
build-model DeepSeek-R1-Distill-Qwen-1.5B q4f16_0 deepseek_r1_qwen "--prefill-chunk-size 256 --context-window-size 4096"
build-model DeepSeek-R1-Distill-Llama-8B q4f16_0 deepseek_r1_llama "--prefill-chunk-size 256 --context-window-size 4096"
#build-model DeepSeek-R1-Distill-Qwen-7B q4f16_0 deepseek_r1_qwen "--prefill-chunk-size 256 --context-window-size 4096"

Copy-Item -Path "./dist/libs/*" -Destination "$MODEL_ARTIFACTS_PATH/dist/libs"

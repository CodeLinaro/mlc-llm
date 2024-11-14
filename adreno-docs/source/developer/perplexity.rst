Perplexity
==========

Perplexity is a critical metric that measures the uncertainty or "confusion" of a model when predicting the next token in a sequence. A lower perplexity indicates that the model is more confident in its predictions, while a higher perplexity suggests greater uncertainty.

Perplexity helps researchers and developers quantify how well LLMs are performing on various text-based tasks, such as text generation, translation, and language understanding. By providing a clear metric for model evaluation, Perplexit aids in optimizing model architectures and improving overall performance in natural language processing (NLP) applications.


Assumption:
    You have model-lib generated using: 

.. code-block:: python

   python3 -m mlc_llm compile HF_LLM_MODEL_PATH --quantization q4f16_0 --device DEVICE --output OUTPUT --perplexity True

    --perplexity Flag generate model lib that support perplexity math. 
.. list-table:: Operating system options
   :widths: 40 30 30
   :header-rows: 1

   * - Option
     - Linux
     - Windows
   * - DEVICE
     - "opencl"
     - "windows:adreno_x86"

For Example for Hamoa:

.. code-block:: python

   python3 -m mlc_llm compile weights/Llama-2-7b-chat-hf --quantization q4f16_0 --device windows:adreno_x86 --output libs/Llama-2-7b-chat-hf-perp.dll --perplexity True

Steps to calculate PPL using our implementation:

1. Prepare preprocessed input file for the LLM you want to calculate PPL for. 

.. code-block:: python

   python3 preprocess_input.py --destination INPUT_FILE_PATH --model_path HF_MODEL_PATH 
   

2. Generate perplexity score for LLM using mlc_calculate_perplexity, it runs mlc model on hardware of your  choice for all the input vectors we have generated in step 1, and in the end calculate the log probabilities and cumulative perplexity score. Use the command below for the same

.. code-block:: python

   python3 calculate_perplexity.py --model MLC_Converted_weights --model-lib MLC_generated_lib --device DEVICE --input-path PATH_OF_INPUT_TEXT_FILE

For Example for Hamoa:

.. code-block:: python

   python3 calculate_perplexity.py --model dist/Llama-2-7b-chat-hf-q4f16_0-MLC --model-lib libs/Llama-2-7b-chat-hf-perp.dll  --device windows:adreno_x86 --input-path perplexity_input.txt
                                
3. Additional check/ Comparison script for mlc solution vs Pytorch, hf_calculate_perplexity calculates the score for LLM using pytorch for Full precision and 4Bitquanization supported by pytorch via bitsandbytes.
Note: This script is designed to run the pytorch for exact input vectors we fed to mlc for comparison. 

.. code-block:: python

   python3 hf_calculate_perplexity.py --input-path INPUT_PATH --model-path HF_MODEL_PATH

For Example for Hamoa:

.. code-block:: python

    python3 hf_calculate_perplexity.py --input-path perplexity_input.txt --model-path weights/Llama-2-7b-chat-hf

Dependencies
~~~~~~~~~~~~

Required python packages:

- torch
- datasets
- transformers
- bitsandbytes


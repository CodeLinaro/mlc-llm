Sample
======

Lets consider a sample model which will be used to demonstrate the entire flow of model compilation, target setup, deploy on target and run the same.
Large language models can be downloaded from Huggingface (https://huggingface.co) or any other sources.

Model compilation entirely happen through python utility ``mlc_llm`` offered by mlc_llm. This process is common for windows or Linux with minor changes.

Given a ``Meta-Llama-3-8B-Instruct`` located under a folder ``Meta-Llama-3-8B-Instruct``

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


Operating system specific options are listed below

.. list-table:: Operating system options
   :widths: 40 30 30
   :header-rows: 1

   * - Option
     - Linux
     - Windows
   * - DEVICE_CONFIG
     - "android:adreno-so"
     - "windows:adreno_x86"

For example ``Meta-Llama-3-8B-Instruct`` model can be compiled for Adreno GPU Targets (Android targets) on Linux as 

::

  # Generate Config
  python -m  mlc_llm gen_config \
         ./dist/models/Llama-2-7b-chat-hf \
         --quantization q4f16_0 \
         --conv-template llama-2 \
         --prefill-chunk-size 256  \
         -o ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC
 
  # Quantize Parameters
  python3 -m mlc_llm convert_weight \
          ./dist/models/Llama-2-7b-chat-hf \
          --quantization q4f16_0 \
          -o ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC
 
  # Compile model for Linux / Android Adreno GPU target
  python3 -m mlc_llm compile \
          ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC/mlc-chat-config.json \
          --device  android:adreno-so \
          -o ./dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno.so

The artifacts we need pick here are the quantized weights located at ``./dist/Llama-2-7b-chat-hf-q4f16_0-MLC`` and the model library located at ``./dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno.so``.



Same can be compiled on Windows environment targetting Adreno GPU on windows as 

::

  # Generate Config
  python -m  mlc_llm gen_config \
         ./dist/models/Llama-2-7b-chat-hf \
         --quantization q4f16_0 \
         --conv-template llama-2 \
         --prefill-chunk-size 256  \
         -o ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC
 
  # Quantize Parameters
  python3 -m mlc_llm convert_weight \
          ./dist/models/Llama-2-7b-chat-hf \
          --quantization q4f16_0 \
          -o ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC
 
  # Compile model for Windows Adreno GPU Target
  python3 -m mlc_llm compile \
          ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC/mlc-chat-config.json \
          --device windows:adreno_x86 \
          -o ./dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno.dll
 

Similarly, windows artifacts we need to pick are quantized weights located at ``./dist/Llama-2-7b-chat-hf-q4f16_0-MLC``  and model library located at ``./dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno.dll``.

Technically, the quantized weights are same for any Adreno GPU target as we use same quantization across.


Deploy & Run on Adreno GPU Target
---------------------------------

Model running very similar using the native cli for both Linux(or Android) and Windows targets. Where as Windows additionally support python cli based run too.

Linux or Android
~~~~~~~~~~~~~~~~

We use precompiled target cli utils ``mlc_llm-utils-lihux-arm64.tar.bz2``. It has the cli tool ``mlc_cli`` and it's dependencies.

::

  mlc_llm-utils-linux-arm64-v001
    ├── bin
    │   └── mlc_cli_chat
    └── lib
        ├── libmlc_llm_module.so
        ├── libmlc_llm.so
        └── libtvm_runtime.so

Push these contents to Adreno GPU target running Linux or Android operating system

Also, push build artifacts from compiled artifacts ``dist/Llama-2-7b-chat-hf-q4f16_0-MLC`` and ``dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno.so`` host to target.

Now, below command can launch the chat on cli

::

  LD_LIBRARY_PATH=./libs ./mlc_cli_chat --model <PATH to Llama-2-7b-chat-hf-q4f16_0-MLC> --model-lib <PATH to Llama-2-7b-chat-hf-q4f16_0-adreno.so> --device opencl


Windows
~~~~~~~

Windows supports Python way of running compiled model as well as native cli approach.

Native Cli
^^^^^^^^^^

We use precompiled target cli utils ``mlc_llm-utils-win-x86.tar.bz2``. It has the cli tool ``mlc_cli.exe`` and it's dependencies as listed below.

::

  mlc_llm-utils-win-x86-v001
    bin
    ├── mlc_cli_chat.exe
    ├── mlc_llm.dll
    ├── mlc_llm_module.dll
    └── tvm_runtime.dll


Now, push build artifacts from compiled artifacts ``dist/Llama-2-7b-chat-hf-q4f16_0-MLC`` and ``dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno.dll`` host to target.

Also, on target below command can launch it for interactive chat

::

  mlc_cli_chat.exe --model <PATH to Llama-2-7b-chat-hf-q4f16_0-MLC> --model-lib <PATH to Llama-2-7b-chat-hf-q4f16_0-adreno-accl.so> --device opencl


Python Cli
^^^^^^^^^^

Prepare the target Hamoa device same as windows host as described below

Install Anaconda from https://docs.anaconda.com/anaconda/install/windows/

Create a anaconda environment with below configuration.

::

  conda create -n mlc-venv -c conda-forge "llvmdev=15" "cmake>=3.24" git rust numpy decorator psutil typing_extensions scipy attrs git-lfs python=3.12 onnx clang_win-64 
  conda activate mlc-venv


Download MLC-LLM (Windows) package from Releases

Now, install the package as shown below

::

  pip install tvm_adreno_cpu_v001-0.18.dev0-cp312-cp312-win_amd64.whl
  pip install mlc_llm_adreno_cpu_v001-0.1.dev0-cp312-cp312-win_amd64.whl

Check the installation status as

::

  python -c "import tvm; print(tvm.__path__)"\
  python -c "import mlc_llm; print(mlc_llm.__path__)"


Now, copy the build artifacts ``dist/Llama-2-7b-chat-hf-q4f16_0-MLC`` and ``dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno-win.so`` from host to Hamoa device.

Under Anaconda shell with environment ``mlc-venv`` execute below command

::

  python -m mlc_llm  chat --device opencl --model-lib ./dist/libs/Llama-2-7b-chat-hf-q4f16_0-adreno-win-accl.so  ./dist/Llama-2-7b-chat-hf-q4f16_0-MLC/



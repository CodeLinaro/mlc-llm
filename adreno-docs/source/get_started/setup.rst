Setup
=====

Here we are detailing MLC-LLM environment for Adreno GPU for each operating system.

Linux
-----

For Linux operating system we need linux wheel and linux-arm64 utils from Releases.
Linux wheel is available in cpu and cuda flavours. Choose cuda if you are intended to use nvidia hardware too for any reference experiments.
Cuda support ehnances the weight qualtization while compilation.

::

  mlc_llm-utils-linux-arm64-v001.tar.bz2
  mlc_llm_adreno_cpu_v001-0.1.dev0-cp38-cp38-manylinux_2_28_x86_64.whl
  mlc_llm_adreno_cuda_v001-0.1.dev0-cp38-cp38-manylinux_2_28_x86_64.whl
  tvm_adreno_mlc_cpu_v001-0.18.dev0-cp38-cp38-manylinux_2_28_x86_64.whl
  tvm_adreno_mlc_cuda_v001-0.18.dev0-cp38-cp38-manylinux_2_28_x86_64.whl


Now, install the package as shown below

::

  pip install tvm_adreno_mlc_cpu_v001-0.18.dev0-cp38-cp38-manylinux_2_28_x86_64.whl
  pip install mlc_llm_adreno_cpu_v001-0.1.dev0-cp38-cp38-manylinux_2_28_x86_64.whl

Check the installation status as

::

  python -c "import tvm; print(tvm.__path__)"
  python -c "import mlc_llm; print(mlc_llm.__path__)"


You may also download the utils and extract as below

::

  mlc_llm-utils-linux-arm64-v001
    ├── bin
    │   └── mlc_cli_chat
    └── lib
        ├── libmlc_llm_module.so
        ├── libmlc_llm.so
        └── libtvm_runtime.so

Utils contains pre compiled target binaries that can be used to run the compiled large language model natively on Adreno GPU target.


Windows
-------

For windows operating system we need windows wheel and win-arm64 utils from Releases.

::

  mlc_llm-utils-win-x86-v001.zip
  mlc_llm_adreno_cpu_v001-0.1.dev0-cp312-cp312-win_amd64.whl
  tvm_adreno_cpu_v001-0.18.dev0-cp312-cp312-win_amd64.whl


Now, install the package as shown below

::

  pip install tvm_adreno_cpu_v001-0.18.dev0-cp312-cp312-win_amd64.whl
  pip install mlc_llm_adreno_cpu_v001-0.1.dev0-cp312-cp312-win_amd64.whl

Check the installation status as

::

  python -c "import tvm; print(tvm.__path__)"\
  python -c "import mlc_llm; print(mlc_llm.__path__)"


You may also download the utils and extract as below

::

  mlc_llm-utils-win-x86-v001
    bin
    ├── mlc_cli_chat.exe
    ├── mlc_llm.dll
    ├── mlc_llm_module.dll
    └── tvm_runtime.dll



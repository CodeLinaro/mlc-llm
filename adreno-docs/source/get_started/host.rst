Host Setup
==========

Working with MLC-LLM for Adreno GPU need some pre requisites depending on the host operating system.
We suport development environment to be on Linux for Android or Linux based Adreno GPU targets and Windows for windows based Adreno GPU targets like Snapdragon Windows Copilot+ PC.

Linux
-----

Recommended to use Ubuntu 20.04 or higher. We recommend anaconda. Refer https://docs.anaconda.com/anaconda/install/linux/ for installing Anaconda.

Now, create a anaconda environment with below configuration.

::

    conda create -n mlc-venv -c conda-forge "llvmdev=15" "cmake>=3.24" git rust numpy decorator psutil typing_extensions scipy attrs git-lfs gcc=10.4 gxx=10.4 python=3.8
    conda activate mlc-venv


Additional Dependencies
~~~~~~~~~~~~~~~~~~~~~~~

::

    pip install torch==2.2.0 torchvision==0.18.0 torchaudio==2.3.0


Windows
-------
Install Anaconda from https://docs.anaconda.com/anaconda/install/windows/ 

::

    conda create -n mlc-venv -c conda-forge "llvmdev=15" "cmake>=3.24" git rust numpy==1.26.4 decorator psutil typing_extensions scipy attrs git-lfs python=3.12 onnx clang_win-64 
    conda activate mlc-venv

Additional Dependencies
~~~~~~~~~~~~~~~~~~~~~~~

::

    pip install torch==2.2.0 torchvision==0.18.0 torchaudio==2.3.0


TVM module compilation need GCC compiler > 7.1. You may install it from MinGW Distro - nuwen.net
We need to explicitly add this path to system PATH. Run on Windows Target



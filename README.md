# Radio Module v2

## Overview
Second version of radio module for rocket flight computer.

## Getting Started

### Download
Run the following command to clone repo and initialize all submodules: ```git clone --recursive https://github.com/RocketScienceOfficial/radio-module-v2```

If you already downloaded repo without ```--recursive``` flag, run: ```git submodule update --init --recursive```

### Environment
You have two ways to setup the project:

<ins>**1. Docker**</ins>

Build image: ```docker build -t radio-module-v2 .```

Run container: ```docker run --rm -it -v %cd%:/env radio-module-v2```

<ins>**2. Manual**</ins>

Install required software:
1. CMake
2. ARM Toolchain
3. A build system (Ninja, etc...)
4. A compiler (GCC, Clang, MinGW, etc...)

### Build
Project is done in CMake, so create build directory and run: ```cmake ..``` in it. You can also specify your build system (e.g. Ninja) or set some variables (list of them is below). Then you can build your project: ```make``` (Linux) or for example ```ninja```.
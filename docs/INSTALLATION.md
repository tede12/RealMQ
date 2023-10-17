# Installation

### Prerequisites

- C compiler (e.g., gcc)
- Python (for generating graphs)

##  Getting Started

1. Clone the repository

```bash
git clone https://www.github.com/tede12/RealMQ
```

2. Run the install script

```bash
chmod +x install.sh && ./install.sh
```

### Compile draft zmq for using UDP features

Unfortunately the draft api is not enabled by default in the zmq library. To enable it you need to compile the library by yourself.
The following steps are for compiling the library on Linux and MacOS. For more information you can check the official documentation [here](https://github.com/zeromq/libzmq).

- **Linux version**

    ```bash
        sudo apt-get install libtool pkg-config build-essential autoconf automake
        git clone https://github.com/zeromq/libzmq
        cd libzmq
        ./autogen.sh
        ./configure --with-libsodium --enable-drafts=yes
        make
        sudo make install
    ```
  After compiling if the command `pkg-config --cflags libzmq` shows `-I/usr/local/include -DZMQ_BUILD_DRAFT_API` then it
  is successfully compiled and draft api is enabled.

    - **MacOS version**

        ```bash
            brew install qt@5 automake autoconf libtool pkg-config
            cd # In this example we compile in the home directory (if you want to change the directory, you must change the path in the CMakelists.txt file)
            git clone https://github.com/zeromq/libzmq
            LDFLAGS="-L/opt/homebrew/opt/qt@5/lib" ./configure --disable-dependency-tracking --with-libsodium --enable-drafts=yes --without-documentation
            make
            sudo make install
        ```
      After compiling if the program do not find the DYLD_LIBRARY_PATH then you must add it to the path:
        ```bash
            export DYLD_LIBRARY_PATH=~/libzmq/src/.libs:$DYLD_LIBRARY_PATH
        ```
      If you want to add it permanently to dyld library path the you need to update the lib in `/usr/local/lib`:
        ```bash
            sudo cp ~/libzmq/src/.libs/libzmq.5.dylib /usr/local/lib/
        ```
      After this command you should be able to see the library in the `/usr/local/lib` folder.
      ```bash
        ls /usr/local/lib/ | grep libzmq
      ```
      To be sure everything is working fine you should update dyld cache:
      ```bash
      sudo update_dyld_shared_cache
      ```
      At the end you should be able to run the executable without any problem.

## Usage

1. Compile the project using CMake:

```bash
mkdir build
cd build
cmake .. 
make
```

2. Run the server and client executables:

```bash
# client and server executables are located in the build directory

# Start the server node
./realmq_server

# (optional) run the plot.py script to graph the results
source ../venv/bin/activate && python3 ../latency_plot.py

# Start the client node
./realmq_client
```
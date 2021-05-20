# Cable: CMake Bootstrap Library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

set(WASI_SDK_PATH "/wasi-sdk-12.0/")

set(CMAKE_SYSTEM_PROCESSOR wasm32-wasi)
set(CMAKE_SYSTEM_NAME Linux) # Pretty close.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_INIT "--target=wasm32-wasi --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot")
set(CMAKE_C_FLAGS_INIT "--target=wasm32-wasi --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot")

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_FIND_ROOT_PATH ${WASI_SDK_PATH})

#[[
--------------------------------------------------------------------------------
Copyright © 2017 by Federico Busato

XLib is provided under the terms of The MIT License (MIT):

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------------------------
@author Federico Busato
         Univerity of Verona, Dept. of Computer Science
         federico.busato@univr.it
@date July, 2017
@version v1.3
]]

project(XLib-v1.2)
cmake_minimum_required(VERSION 3.5)

#===============================================================================
#######################
# CMAKE CONFIGURATION #
#######################

set(UNUSED_COMPILER_OPTIONS OFF)    #unused compiler flags
set(MARCH_COMPILER_OPTIONS OFF)     #march compiler flag
#EXTERNAL VARIABLE
#set(WARNING_FLAGS ON)
#set(CUB_LIBRARY   <PATH>)
#set(CUDAHC <PATH>)
#===============================================================================
###################
# INCLUDE AND SRC #
###################


if (CMAKE_BUILD_TYPE STREQUAL "")
    set (CMAKE_BUILD_TYPE Release)
endif()

if (QUIET_SUBMODULE)
    message("\n==> Test on cmake v" ${CMAKE_VERSION}
            "  System: " ${CMAKE_SYSTEM})
    message("\n==> Selected CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE} "\n")
endif()

if (    (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    AND (NOT CMAKE_BUILD_TYPE STREQUAL "ReleaseEx")
    AND (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    AND (NOT CMAKE_BUILD_TYPE STREQUAL "Info"))
    message(FATAL_ERROR
    "\nwrong BUILD_TYPE: \"-DCMAKE_BUILD_TYPE=[ Release | Debug | Info |"
    " ReleaseEx]\"")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#===============================================================================
#######################
# HOST COMPILER FLAGS #
#######################
include(CheckCXXCompilerFlag)

#CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
if (COMPILER_SUPPORTS_CXX14)
    add_compile_options("-std=c++14")
#else()
#if (COMPILER_SUPPORTS_CXX11)
#    add_compile_options("-std=c++11")
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} has no C++14 support.")
endif()

CHECK_CXX_COMPILER_FLAG("-Qunused-arguments" UNUSED_FLAG)
if (UNUSED_FLAG)
    add_compile_options("-Qunused-arguments")
endif()

#if (CMAKE_CXX_COMPILER_ID EQUAL "Clang")
#    add_compile_options("-Wno-c++14-extensions")
#endif()

if (WARNING_FLAGS)
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        add_compile_options("-Weverything")
        add_compile_options("-Wno-c++98-compat")
        add_compile_options("-Wno-c++98-compat-pedantic")
        add_compile_options("-Wno-padded")
        add_compile_options("-Wno-undefined-func-template")
        #add_compile_options("-Wno-documentation-unknown-command")
        #add_compile_options("-Wno-weak-template-vtables")
        #add_compile_options("-Wno-documentation")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        add_compile_options("-Wall")
        add_compile_options("-Wextra")
        add_compile_options("-Wpedantic")
    endif()
endif()

#===============================================================================
#########################
# OPENMP COMPILER FLAGS #
#########################

if (OPENMP_FOUND)
    add_compile_options("${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}")
    message("OpenMP: " ${OpenMP_CXX_SPEC_DATE})
endif()

#===============================================================================
#######################
# CUDA COMPILER FLAGS #
#######################

if (CUDA_FOUND)

set(CUDA_PROPAGATE_HOST_FLAGS OFF)

if ((${CUDA_VERSION_MAJOR} LESS 9))
    message(FATAL_ERROR "\nRequired Cuda Toolkit >= 9   Founded: Cuda Toolkit"
                        " v${CUDA_VERSION_MAJOR}.${CUDA_VERSION_MINOR}")
endif()

find_library(
    DEVRT_LIBRARY cudadevrt
    HINTS  ${CUDA_TOOLKIT_ROOT_DIR}/lib64
)


if (DEFINED CUDAHC)
    if (QUIET_SUBMODULE)
        message("\n==> Cuda Host compiler: " ${CUDAHC})
    endif()
    set( CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-ccbin=${CUDAHC}" )
else()
    execute_process(COMMAND bash "-c" "gcc --version | head -n 1"
                    OUTPUT_VARIABLE CUDAHC)
    if (QUIET_SUBMODULE)
        message("\n==> Cuda Host compiler: ${CUDAHC}"
                "    (-DCUDAHC=<path_to_host_compiler>)")
    endif()
endif()

if (CUB_LIBRARY)
    set( CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-DCUB_WRAPPER" )
    include_directories(${CUB_LIBRARY})
endif()

if (CMAKE_CXX_COMPILER_ID EQUAL "Clang")
    add_compile_options("-Wno-user-defined-literals")
endif()

if (WARNING_FLAGS_CUDA)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

set(DEVICE_WARNING_EXTRA
"-Weverything,\
-Wno-c++98-compat,\
-Wno-padded,\
-Wno-c++98-compat-pedantic,\
-Wno-documentation-unknown-command,\
-Wno-documentation")

set( CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}
                     "-Xcompiler=${DEVICE_WARNING_EXTRA}")
endif()
endif()

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-std=c++14" "--expt-extended-lambda"
    "--expt-relaxed-constexpr"
    "-Xcompiler=-Wno-unknown-pragmas"
    -Xcudafe \\\"--diag_suppress=initialization_not_reachable\\\"
    -Xcudafe \\\"--diag_suppress=code_is_unreachable\\\"
    -isystem /usr/local/cuda/include
    -isystem /tmp)

#===============================================================================
######################
# COMPUTE CABABILITY #
######################

set(GPU_ARCH "" CACHE STRING "compute capability")
set(SM "" CACHE STRING "streaming multiprocessor number")

if(GPU_ARCH STREQUAL "")
    if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/getDeviceInfo)
        execute_process(COMMAND ${CUDA_TOOLKIT_ROOT_DIR}/bin/nvcc
                                    -Wno-deprecated-gpu-targets
                                    ${CMAKE_CURRENT_LIST_DIR}/getDeviceInfo.cu
                                    -o ${CMAKE_CURRENT_LIST_DIR}/getDeviceInfo)
    endif()
    execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/getDeviceInfo
                    OUTPUT_VARIABLE OUT_VARIABLE)

    list(GET OUT_VARIABLE 0 GPU_ARCH)
    list(GET OUT_VARIABLE 1 SM)
endif()
#-------------------------------------------------------------------------------
if (QUIET_SUBMODULE)
if(NOT SM STREQUAL "")
    message("\n==> GPU architecture: CC ${GPU_ARCH}"
            " with ${SM} streaming multiprocessors")

    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-DSM=${SM}")
else()
    message("\n==> GPU architecture: ${GPU_ARCH}")
endif()
endif()
#-------------------------------------------------------------------------------

if (GPU_ARCH STREQUAL "30")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_30" "-DARCH=300")
elseif (GPU_ARCH STREQUAL "32")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_32" "-DARCH=320")
elseif (GPU_ARCH STREQUAL "35")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_35" "-DARCH=350")
elseif (GPU_ARCH STREQUAL "37")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_37" "-DARCH=370")
elseif (GPU_ARCH STREQUAL "50")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_50" "-DARCH=500")
elseif (GPU_ARCH STREQUAL "52")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_52" "-DARCH=520")
elseif (GPU_ARCH STREQUAL "53")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_53" "-DARCH=530")
elseif (GPU_ARCH STREQUAL "60")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_60" "-DARCH=600")
elseif (GPU_ARCH STREQUAL "61")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_61" "-DARCH=610")
elseif (GPU_ARCH STREQUAL "62")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_62" "-DARCH=620")
elseif (GPU_ARCH STREQUAL "70")
    list(APPEND CUDA_NVCC_FLAGS "-arch=sm_70" "-DARCH=700")
else()
    message(FATAL_ERROR "\nplease select GPU architecture:
    \"cmake -DARCH=[ 30 | 32 | 35 | 37 | 50 | 52 | 53 | 60 | 61 | 62 | 70 ]\"")
endif()

#===============================================================================
######################
# BUILD OPTIONS CUDA #
######################

if (UNUSED_FLAG AND UNUSED_COMPILER_OPTIONS)
    set(D_QUNUSED "-Qunused-arguments," )
endif()
if (ARCH_FLAG AND MARCH_COMPILER_OPTIONS)
    set(D_MARCH "-march=native,")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "--use_fast_math" "-DNDEBUG"
    "-Xcompiler=-O3,${D_QUNUSED}${D_MARCH}-mtune=native,-funroll-loops,-DNDEBUG")

elseif (CMAKE_BUILD_TYPE STREQUAL "ReleaseEx")
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "--use_fast_math" "-DNDEBUG"
"-Xcompiler=-O3,${D_QUNUSED}${D_MARCH}-mtune=native,-funroll-loops,-DNDEBUG,\
-DNO_CHECK_CUDA_ERROR")

elseif (CMAKE_BUILD_TYPE STREQUAL "Info")
    file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/build/TMP)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-lineinfo" "-keep" "--use_fast_math"
        "-DNDEBUG"
         "--resource-usage"
        "-keep-dir" "${PROJECT_SOURCE_DIR}/build/TMP"
        "-Xcompiler=-DNDEBUG"
    )

elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-g" "-G")
else()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE not defined")
endif()

endif() #end cuda

#===============================================================================
include(ProcessorCount)
ProcessorCount(THREADS)

if (QUIET_SUBMODULE)

if(THREADS EQUAL 0)
    message(FATAL_ERROR "==> Get Number of CPU threads failed!\n")
else()
    message("\n==> Number of CPU threads: " ${THREADS} "\n")
endif()

endif()

#===============================================================================
##########
# OPENMP #
##########
FIND_PACKAGE(OpenMP)

if (OPENMP_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}\
 -DMAX_THREADS=${THREADS}")
    #message("OPENMP Specification date: " ${OpenMP_CXX_SPEC_DATE})

    if (CUDA_FOUND)
        set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-DMAX_THREADS=${THREADS}"
            "-Xcompiler=${OpenMP_CXX_FLAGS}")
    endif()

endif()

#===============================================================================
######################
# BUILD OPTIONS HOST #
######################

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb3")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()

if (CMAKE_CXX_COMPILER_ID EQUAL "Clang")

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} "-ggdb")

set(CMAKE_CXX_FLAGS_SANITAZE "-O1 -g -fsanitize=address -fno-omit-frame-pointer"
                             " -fno-optimize-sibling-calls -fsanitize=undefined"
                             " -fsanitize=integer")

set(CMAKE_CXX_FLAGS_MEM      "-O2 -g -fsanitize=memory -fno-omit-frame-pointer"
                             "-fsanitize-memory-track-origins")
endif()


execute_process(COMMAND bash "-c" "ld -v | grep -q 'gold';"
                                  "if [ $? = 0 ]; then echo ON; fi"
                OUTPUT_VARIABLE GOLD_LINKER)

if (GOLD_LINKER)
    message("==> Gold Linker\n")
    set(FLTO -flto)
endif()

#-------------------------------------------------------------------------------
CHECK_CXX_COMPILER_FLAG("-march=native" ARCH_FLAG)
if (ARCH_FLAG)
    set(H_MARCH "-march=native")
else()
    set(H_MARCH "-mtune=native")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Info")
    set(CMAKE_CXX_FLAGS "-O3 ${H_MARCH} ${FLTO} \
    -funroll-loops") # -DNDEBUG")
    #Not all compilers support -march=native for all architecuteres -> -mtune

elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "-O3 ${H_MARCH} ${FLTO} \
    -funroll-loops -DNDEBUG")

elseif (CMAKE_BUILD_TYPE STREQUAL "ReleaseEx")
    set(CMAKE_CXX_FLAGS "-O3 ${H_MARCH} ${FLTO} \
    -funroll-loops -DNDEBUG -DNO_CHECK_CUDA_ERROR")

elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "-g")
else()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE not defined")
endif()

#===============================================================================
#################
# CUSTOM TARGET #
#################
if (QUIET_SUBMODULE)

add_custom_target(rm rm -rf * COMMENT "Clear build directory")

add_custom_target(ptx cp ${PROJECT_SOURCE_DIR}/build/TMP/*.ptx
                         ${PROJECT_SOURCE_DIR}/build
                #COMMAND nvdisasm -c -g -sf
                #         ${PROJECT_SOURCE_DIR}/build/TMP/*.cubin > p.txt
                  COMMENT "PTX")

add_custom_target(valgrind cp ${CMAKE_CURRENT_LIST_DIR}/valgrind.sh
                              ${CMAKE_CURRENT_LIST_DIR}/cuda.supp
                              ${PROJECT_SOURCE_DIR}/build
                  COMMAND chmod +x ${PROJECT_SOURCE_DIR}/build/valgrind.sh
                  COMMENT "VALGRIND")

add_custom_target(update rm -rf *
                  COMMAND cmake ..
                  COMMAND make -j ${THREADS}
                  COMMENT "UPDATE RELEASE")

add_custom_target(update_debug rm -rf *
                  COMMAND cmake -DCMAKE_BUILD_TYPE=Debug ..
                  COMMAND make -j ${THREADS}
                  COMMENT "UPDATE DEBUG")

add_custom_target(update_info rm -rf *
                  COMMAND cmake -DCMAKE_BUILD_TYPE=Info ..
                  COMMAND make -j ${THREADS}
                  COMMENT "UPDATE INFO")

add_custom_target(update_ex rm -rf *
                  COMMAND cmake -DCMAKE_BUILD_TYPE=ReleaseEx ..
                  COMMAND make -j ${THREADS}
                  COMMENT "UPDATE ReleaseEx")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

add_custom_target(update_sanitize rm -rf *
                  COMMAND cmake -DCMAKE_BUILD_TYPE=Sanitaze ..
                  COMMAND make -j ${THREADS}
                  COMMENT "UPDATE SANITAZE")

add_custom_target(update_mem rm -rf *
                  COMMAND cmake -DCMAKE_BUILD_TYPE=Mem ..
                  COMMAND make -j ${THREADS}
                  COMMENT "UPDATE MEM")
endif()
endif()

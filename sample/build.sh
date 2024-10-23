#!/bin/bash

module purge

module load intel/2020b
module load CMake/3.12.1
module load GCCcore/8.3.0
module load PAPI/6.0.0

cmake \
    -Dcaliper_DIR=/scratch/group/csce435-f24/Caliper/caliper/share/cmake/caliper \
    -Dadiak_DIR=/scratch/group/csce435-f24/Adiak/adiak/lib/cmake/adiak \
    .

make
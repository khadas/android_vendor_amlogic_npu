Build Vivante OpenCL Driver

Contents

1. Files layout in OCL-addon package
2. Build instruction

1. Files layout in OCL-addon package
====================================
<AQROOT>/
    |
    +---ReadMe_OpenCL.txt             : ReadMe file for OpenCL driver build
    |
    |
    +---compiler
    |   |
    |   \---libCLC                    : OpenCL compiler
    |
    \---driver
        |
        \---khronos
            |
            \---libCL
                |
                +---driver            : OpenCL driver;


2. Build instruction
====================

  1) Make sure you have installed Vivante standard GALVIP driver package;

     tar xzf VIVANTE_GALVIP_Unified_Src_drv_<version>.tgz -C <PROJECTS_DIR>

  3) Install Vivante OpenCL driver add-on: VIVANTE_GALVIP_Unified_Src_drv_OCL-addon_<version>.tgz.

     cd <PROJECTS_DIR>
     tar zxvf VIVANTE_GALVIP_Unified_Src_drv_OCL-addon_<version>.tgz

  4) Enable USE_OPENCL build option in <PROJECTS_DIR>/makefile.linux.def
        USE_OPENCL ?= 1

     Other build option please refer to the discription in ReadMe_Linux.txt;

  5) Build Vivante driver.
     Modify <PROJECTS_DIR>/driver_build_samples.sh to setup build evironment, then call it;

  6) Check out build result.
     By default, openCL driver and frontend driver will be save to:
        $AQROOT/build/sdk/drivers/libOpenCL.so
        $AQROOT/build/sdk/drivers/libVivanteOpenCL.so
        $AQROOT/build/sdk/drivers/libCLC.so
        $AQROOT/build/sdk/drivers/libLLVM_viv.so


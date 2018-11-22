Linux Build for Vivante's Graphics Drivers

Contents

1. Quick start
2. Prerequisits
3. Build
4. Run the applications
5. Specific stuffs

1. Quick start
==============

1) Uncompress the source code, and set the root directory of driver souce code.

   # Please make sure driver and test soruce package are unzipped in different directory.
   # <PROJECTS_DIR> must be a full path.
   export PROJECTS_DRIVER_DIR=<PROJECTS_DIR>/DRIVER
   mkdir -p $PROJECTS_DRIVER_DIR
   tar zxvf VIVANTE_GALVIP_Unified_Src_drv_<version>.tgz -C $PROJECTS_DRIVER_DIR.

   # Set the root directory of driver souce code.
   export AQROOT=$PROJECTS_DRIVER_DIR

2) Set the envionment variables.

   # Please make sure there is NO trailing '/' in AQARCH.
   export AQARCH=<PROJECTS_DRIVER_DIR>/arch/XAQ2

   # Set cross compile toolchain.
   export CROSS_COMPILE=<CROSS_COMPILER_PREFIX>

   # Set the Linux kernel directory.
   export KERNEL_DIR=<PATH_TO_LINUX_KERNEL_SOURCE>

   # Set X11 installation path.
   # If you are not using X11 as the GUI system, this variable is not necessary.
   export X11_ARM_DIR=<PATH_TO_X11_STUFF>

   # Set DirectFB installation path.
   # If you are not building gfx driver, this variable is not necessary.
   export DFB_DIR=<PATH_TO_DIRECTFB_INSTALLATION_DIRECTORY>

   # Set the toolchain path.
   export TOOLCHAIN=<PATH_TO_TOOLCHAIN_TOPDIR>

   # Set the static library path which contains libc.a, etc.
   # If you are not building the package with static link, this variable is not necessary.
   export LIB_DIR=<PATH_TO_STATIC_LIBRARIES>

   # The binararies will be installed at this directory.
   # By default, SDK_DIR=$AQROOT/build/sdk, you can modify it as following:
   DRIVER_SDK_DIR=<PATH_TO_INSTALL_BINARIES>
   export SDK_DIR=$DRIVER_SDK_DIR

   # Add the toolchain to the PATH.
   export PATH=$TOOLCHAIN/bin:$PATH

   # We use arm-2010q1 toolchain to compile this package.

3) Build.

   cd $AQROOT

   make -f makefile.linux
   make -f makefile.linux install

   The binaries are installed at <SDK_DIR>.
   More build options please see 'Build commands'.

4) Run the applications.

   Move to the target board.

   a) Copy <SDK_DIR> to the target board.

   b) Create device node.
      mknod /dev/galcore c 199 0

   c) Insert the kernel driver.
      insmod <SDK_DIR>/drivers/galcore.ko registerMemBase=<REG_MEM_BASE> irqLine=<IRQ> contiguousSize=<CONTIGUOUS_MEM_SIZE>

   d) Set envionment variable.

      export LD_LIBRARY_PATH=<SDK_DIR>/drivers

   e) Run the application.

	  eg. Run tutorial1.

	  cd <SDK_DIR>/samples/tutorial
	  ./tutorial1

2. Prerequisits
===============

1) Build the Linux kernel

   Before building the driver, you need a pre-compiled Linux source code tree.

   The driver needs a large piece of contiguous physical memory.
   So you should enlarge the memory zone in the kernel.

   You can choose either way to do so depends on how you build the driver.

   a) Build the driver with NO_DMA_COHERENT=1

   You need to enlarge the MAX_ORDER.

   Add "#define MAX_ORDER 15" in linux/include/linux/mmzone.h.

   b) Build the driver with NO_DMA_COHERENT=0 (the default value)

   You should enlarge both the MAX_ORDER and CONSISTENT_DMA_SIZE.

   Add "#define MAX_ORDER 15" in linux/include/linux/mmzone.h.
   Change "#define CONSISTENT_DMA_SIZE SZ_32M" in linux/include/asm-arm/memory.h.

3. Build
=========

1) Build commands.

   a) Build
   make -f makefile.linux <OPTIONS>

   b) Install
   make -f makefile.linux <OPTIONS> install

   c) Clean

   make -f makefile.linux <OPTIONS> clean

   Please see section 3) for the details on OPTIONS.

2) The targets

   There are a lot of modules in the package.

   Use the following the commands to build, install and clean a spcific module.

   a) Build
   make -f makefile.linux <OPTIONS> <MODULE>

   b) Install
   make -f makefile.linux <OPTIONS> <MODULE> V_TARGET=install

   c) Clean

   make -f makefile.linux <OPTIONS> <MODULE> V_TARGET=clean

   MODULE list:

   hal_drv:         Build HAL driver, including galcore.ko, libGAL.so.
   gfx:				Build DirectFB driver.
   egl:             Build EGL dirver.
   oes11_drv:       Build OpenGL ES 1.1 driver.
   oes2x_drv:       Build OpenGL ES 3.0 driver.
   ovg11_drv:       Build OpenVG 1.1 driver.

   Please see the section 3) for the details on OPTIONS.

3) The options

   There are a lot of OPTIONS to control how to build the driver.
   To enable/disable an option, set <OPTION>=<value> in the command line.
    option                 value      description
    -------------------------------------------------
    DEBUG                    0        Disable debugging;deafult value;
                             1        Enable debugging;
    CPU_TYPE           [CPU type]     Set -mcpu=[CPU type] in the build command line.	0

   	                         0        Use the default value - arm920.

    NO_DMA_COHERENT          0        Enable coherent DMA;deafult value;
                             1        Disable coherent DMA;

    ABI                      0        Change application binary interface;default is 0 which means no setting;
                       aapcs-linux    For example, build driver for Aspenite board;

    USE_VDK                  0        Don't use the VDK programming interface;deafult value;
                             1        Use the VDK programming interface;
                                      Don't support static linking;
                                      Supports FBDEV and X11 system;

    EGL_API_FB               0        Use X11 system as the GUI system for the EGL;deafult value;
                             1        Use the FBDEV as the GUI system for the EGL;
                                      Supports static linking with STATLIC_LINK=1 build option;

    gcdSTATIC_LINK           0        Disable static linking;deafult value;
                                      Don't affects on gfx driver.
                             1        Enable static linking;
                                      Don't affects on gfx driver.

    CUSTOM_PIXMAP            0        Don't use special pixmap surface format;deafult value;
                             1        Use special pixmap surface format;
                                      Only affects on EGL driver;

    USE_PLATFORM_DRIVER      1        Set USE_PLATFORM_DRIVER to 1 if Linux kernel version is 2.6.x;default value;
                             0        Set USE_PLATFORM_DRIVER to 0 for some old Linux kernel.

    ENABLE_GPU_CLOCK_BY_DRIVER 1      Enable GPU clock in driver layer for some special boards;
                               0      Do not care GPU clock setting in driver;

    USE_NEW_LINUX_SIGNAL     0        Use traditional signal mechanism; default value;
                             1        Use new signal mechanism;


    gcdFPGA_BUILD            0        To build the driver for the FPGA board, set it to 1, otherwise set it to 0.
                             1

    USE_BANK_ALIGNMENT       1        Enable gcdENABLE_BANK_ALIGNMENT, and video memory is allocated bank aligned.
                                      The vendor can modify _GetSurfaceBankAlignment() and gcoSURF_GetBankOffsetBytes()
                                      to define how different types of allocations are bank and channel aligned.
                             0        Disabled (default), no bank alignment is done.

    BANK_BIT_START           0        Use default start bit of the bank defined in gc_hal_options.h
                     [BANK_BIT_START] Specifies the start bit of the bank (inclusive).
                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;

    BANK_BIT_END             0        Use default end bit of the bank defined in gc_hal_options.h
                     [BANK_BIT_END]   Specifies the end bit of the bank (inclusive);
                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;

    BANK_CHANNEL_BIT         0        Use default channel bit defined in gc_hal_options.h
                   [BANK_CHANNEL_BIT] When set to no-zero, video memory when allocated bank aligned is allocated such
                                      that render and depth buffer addresses alternate on the channel bit specified.
                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled.

    USE_LOADTIME_OPT         1        Enable GLSL compiler load time optimization(default).
                             0        Disable GLSL compiler load time optimization.

   USE_POWER_MANAGEMENT      1        Enable GPU power managment code;
                             0        Disable GPU power management code; Should set it to 0 on FPGA board;

4. Run the applications
=======================

Move to the target board.

1) Copy <SDK_DIR> to the target board.

2) Create the device node.

   mknod /dev/galcore c 199 0

3) Insert the kernel driver.

   There are two kinds of ways to manage the video memory: pre-reserved and
   dynamiclly allocated.

   We must pass the correct parameters to the driver according to these two conditions.

   a) Reserve a piece of memory from the system.

      insmod <SDK_DIR>/drivers/galcore.ko registerMemBase=<REG_MEM_BASE> irqLine=<IRQ> contiguousBase=<CONTIGUOUS_MEM_BASE> contiguousSize=<CONTIGUOUS_MEM_SIZE>

   b) Allocate a piece of memory dynamiclly when the driver boots up.

      insmod <SDK_DIR>/drivers/galcore.ko registerMemBase=<REG_MEM_BASE> irqLine=<IRQ> contiguousSize=<CONTIGUOUS_MEM_SIZE>

4) Set envionment variable.

   export LD_LIBRARY_PATH=<SDK_DIR>/drivers

   Add additional paths to LD_LIBRARY_PATH if you are use other extra libraries.

5) Run the application.

   eg. Run tutorial1.

   cd <SDK_DIR>/samples/tutorial
   ./tutorial1

5. Specific cases
=================

We list some specific cases below for your reference.

1) Build DirectFB accelerator for DirectFB driver

   a) Build DirectFB package.

   Download the source code from www.directfb.org.
   We are using DirectFB-1.4.0 as this release.

   Cross compile the package and install the libraries and head files into <DIRECTFB_DIR>.
   At the same time copy them to the target board at the same directory - <DIRECTFB_DIR>.

   We have attached a script named as build_dfb.sh in <PROJECT_DIR>/release/SW on how to configure and cross compile DirectFB-1.4.0.

   You can take it as a reference.

   b) Build DirectFB accelerator.

   export AQROOT=<PROJECTS_DIR>
   export AQARCH=<PROJECTS_DIR>/arch/XAQ2
   export CROSS_COMPILE=<CROSS_COMPILER_PREFIX>
   export KERNEL_DIR=<PATH_TO_LINUX_KERNEL_SOURCE>
   export DFB_DIR=<DIRECTFB_DIR>
   export TOOLCHAIN=<PATH_TO_TOOLCHAIN_TOPDIR>
   export SDK_DIR=<PATH_TO_INSTALL_BINARIES>
   export PATH=$TOOLCHAIN/bin:$PATH

   # Set the ABI to the same when build the Linux kernel.
   export BUILD_OPTIONS="$BUILD_OPTIONS ABI=aapcs-linux"

   make -f makefile.linux $BUILD_OPTIONS gfx
   make -f makefile.linux $BUILD_OPTIONS gfx V_TARGET=install

   We have attached a build script named as build_sample.sh in <PROJECT_DIR>/release/SW.
   You can take it as a reference.

   The library libdirectfb_gal.so is installed at <SDK_DIR>/drivers.

   c) Run the tests.

   Move to the target board.

   Copy libdirectfb_gal.so to <DFB_DIR>/lib/directfb-1.4-0/gfxdrivers.

   Follow section 3 Run the applications' 1), 2) and 3) to setup the envionment.

   Then edit ~/.directfbrc as the follwing:

		# directfbrc begin

		system=fbdev
		mode=640x480					# change to fit your needs
		desktop-buffer-mode=frontonly	# if no double buffering in frame
										# buffer driver
		depth=16
		pixelformat=RGB16
		#no-hardware					# disable hardware acceleration
		#no-software					# disable software fallbacks

		# directfbrc end

   Set the envionment variable.

   export LD_LIBRARY_PATH=<DIRECTFB_DIR>/lib:$LD_LIBRARY_PATH

   Run the applications.

   e.g.

   cd <SDK_DIR>/samples/gfx

   ./draw_line

   d) Use the configuration file to get more control on GFX.

   You can find a reference configuration file at
   $AQROOT/driver/gfx/gal_config.

   This is the configuration file for Vivante GFX plug-in driver.
   You can use this file to control which primitive is accelerated
   with specific features.

   e.g. If you want to accelerate blit with alpha blending and rotate180
        features, please add the following line in the file.
        blit=alphachannel,coloralpha,rotate180

        Then blit with other features (including xor and src_colorkey etc.)
        are not accelerated by HW. Even blit without any features is not
        accelerated also.
        "none" in the feature list means the rendering primitive without
        any features.

   Following is the full matrix of the primitives and features.

        drawline=none,xor,blend
        drawrectangle=none,xor,blend
        fillrectangle=none,xor,blend
        filltriangle=none,xor,blend
        blit=none,xor,alphachannel,coloralpha,src_colorkey,rotate180
        stretchblit=none,xor,alphachannel,coloralpha,src_colorkey,rotate180

   To use the configuration file, please set envionment variable GAL_CONFIG_FILE
   pointing to this file.

   e.g. For bash user.
        export GAL_CONFIG_FILE=/home/gfx/gal_config

   If you don't set the envionment variable, a default configuration matrix
   will be used.

   The default configuration matrix is listed below.

       fillrectangle=none,xor,blend
       filltriangle=none,xor,blend
       filltriangle=none,xor,blend
       blit=none,xor,alphachannel,coloralpha,src_colorkey,rotate180
       stretchblit=none,xor,alphachannel,coloralpha,src_colorkey,rotate180

   Configuration file has higher priority.


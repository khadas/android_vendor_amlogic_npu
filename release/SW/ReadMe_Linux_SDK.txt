Files layout
============
sdk
|
+---drivers
|      galcore.ko          # kernel HAL driver
|      libGAL.so           # user HAL driver
|      libEGL.so           # EGL driver
|      libGLESv1_CM.so     # OpenGL ES 1.1 driver
|      libOpenVG.so        # OpenVG 1.1 driver
|      libGLESv2.so        # OpenGL ES 3.x driver
|      libGLSLC.so         # OpenGL ES 3.x shading language compiler
|      libVSC.so           # backend of compiler
|      libVDK.so           # Vivante Development Kit (USE_VDK=1)
|
\---include
    |  gc_vdk_types.h
    |  gc_vdk.h
    |
    +---HAL
    |     gc_hal.h
    |     gc_hal_types.h
    |     gc_hal_enum.h
    |     gc_hal_dump.h
    |     gc_hal_base.h
    |     gc_hal_mem.h
    |     gc_hal_rename.h
    |     gc_hal_raster.h
    |     gc_hal_engine.h
    |     gc_hal_options.h
    |     gc_hal_driver.h
    |     gc_hal_profiler.h
    |     gc_hal_statistics.h
    |     gc_hal_version.h
    |
    +---KHR
    |     khrplatform.h
    |
    +---EGL
    |     egl.h
    |     eglext.h
    |     eglplatform.h
    |     eglvivante.h
    |
    +---GLES
    |     gl.h
    |     glext.h
    |     egl.h
    |     glplatform.h
    |     glrename.h
    |     glunname.h
    |
    +---GLES2
    |     gl2.h
    |     glext.h
    |     gl2platform.h
    |     gl2rename.h
    |     gl2unname.h
    |
    +---GLES3
    |     gl3.h
    |     gl31.h
    |     gl32.h
    |     gl3ext.h
    |     gl3platform.h
    |     gl3rename.h
    |     gl3unname.h
    |
    \---VG
          openvg.h
          vgu.h
          vgext.h
          vgplatform.h
          vgrename.h
          vgunname.h

Running applications on the target machine
==========================================

1. Copy the libraries to the target
    On the target machine:
    cp galcore.ko /
    cp libEGL.so libGLESv1_CM.so libGAL.so libGLSLC.so libGLESv2.so /lib

2. Install the kernel driver
    insmod /galcore.ko registerMemBase=<REG_MEM_BASE> irqLine=<IRQ> contiguousSize=<CONTIGUOUS_MEM_SIZE>

    eg. On ARM EB development board:
    insmod /galcore.ko registerMemBase=0x80000000 irqLine=104 contiguousSize=0x400000

3. Run the application
    eg.
    cd $SDK_DIR/samples/vdk; ./tutorial1



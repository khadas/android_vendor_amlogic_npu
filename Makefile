ifeq ($(KERNEL_A32_SUPPORT), true)
KERNEL_ARCH := arm
else
KERNEL_ARCH := arm64
endif

DEFAULT_NPU_KERNEL_MODULES := $(PRODUCT_OUT)/obj/lib_vendor/galcore.ko

MODS_OUT := $(shell pwd)/$(PRODUCT_OUT)/obj/lib_vendor
ifeq (,$(wildcard $(MODS_OUT)))
$(shell mkdir $(MODS_OUT) -p)
endif

KDIR := $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ/
NPU_MODULES := $(shell pwd)/$(PRODUCT_OUT)/obj/npu_modules

VIVANTE_ENABLE_3D                 ?= 1
VIVANTE_ENABLE_2D                 ?= 0
VIVANTE_ENABLE_VG                 ?= 0
VIVANTE_ENABLE_DRM                ?= 0
NO_DMA_COHERENT                   ?= 1
CACHE_FUNCTION_UNIMPLEMENTED      ?= 0
USE_BANK_ALIGNMENT                ?= 0
BANK_BIT_START                    ?= 0
BANK_BIT_END                      ?= 0
BANK_CHANNEL_BIT                  ?= 0
SECURITY                          ?= 0
USE_OPENCL ?= 1
USE_OPENVX                   ?= 1
USE_VXC_BINARY  ?=0
GPU_CONFIG  ?=vipnanoqi_pid0x88
VIVANTE_ENABLE_3D            ?=1
VIVANTE_ENABLE_2D            ?=0
VIVANTE_ENABLE_VG            ?=0
VIVANTE_ENABLE_DRM ?=0
SOC_PLATFORM                 ?= amlogic

EXTRA_CFLAGS1 += -DLINUX -DDRIVER




ifeq ($(PLATFORM_VERSION),10)
CONFIGS := CONFIG_PLATFORM_VERSION=10
else
CONFIGS := CONFIG_PLATFORM_VERSION=9
endif

ifeq ($(FLAREON),1)
EXTRA_CFLAGS1 += -DFLAREON
endif

EXTRA_CFLAGS1 += -DDBG=1 -DDEBUG -D_DEBUG


ifeq ($(NO_DMA_COHERENT),1)
EXTRA_CFLAGS1 += -DNO_DMA_COHERENT
endif

ifeq ($(CONFIG_DOVE_GPU),1)
EXTRA_CFLAGS1 += -D CONFIG_DOVE_GPU=1
endif

ifneq ($(USE_PLATFORM_DRIVER),0)
EXTRA_CFLAGS1 += -D USE_PLATFORM_DRIVER=1
else
EXTRA_CFLAGS1 += -D USE_PLATFORM_DRIVER=0
endif

EXTRA_CFLAGS1 += -D VIVANTE_PROFILER=1
EXTRA_CFLAGS1 += -D VIVANTE_PROFILER_CONTEXT=1

ifeq ($(ENABLE_GPU_CLOCK_BY_DRIVER),1)
EXTRA_CFLAGS1 += -D ENABLE_GPU_CLOCK_BY_DRIVER=1
else
EXTRA_CFLAGS1 += -D ENABLE_GPU_CLOCK_BY_DRIVER=0
endif

ifeq ($(USE_NEW_LINUX_SIGNAL),1)
EXTRA_CFLAGS1 += -D USE_NEW_LINUX_SIGNAL=1
else
EXTRA_CFLAGS1 += -D USE_NEW_LINUX_SIGNAL=0
endif

ifeq ($(USE_LINUX_PCIE),1)
EXTRA_CFLAGS1 += -D USE_LINUX_PCIE=1
else
EXTRA_CFLAGS1 += -D USE_LINUX_PCIE=0
endif

ifeq ($(CACHE_FUNCTION_UNIMPLEMENTED),1)
EXTRA_CFLAGS1 += -D gcdCACHE_FUNCTION_UNIMPLEMENTED=1
else
EXTRA_CFLAGS1 += -D gcdCACHE_FUNCTION_UNIMPLEMENTED=0
endif

EXTRA_CFLAGS1 += -D gcdENABLE_3D=1
EXTRA_CFLAGS1 += -D gcdENABLE_2D=0
EXTRA_CFLAGS1 += -D gcdENABLE_VG=0

ifeq ($(USE_BANK_ALIGNMENT),1)
    EXTRA_CFLAGS += -DgcdENABLE_BANK_ALIGNMENT=1
    ifneq ($(BANK_BIT_START),0)
	        ifneq ($(BANK_BIT_END),0)
	            EXTRA_CFLAGS1 += -D gcdBANK_BIT_START=$(BANK_BIT_START)
	            EXTRA_CFLAGS1 += -D gcdBANK_BIT_END=$(BANK_BIT_END)
	        endif
    endif

    ifneq ($(BANK_CHANNEL_BIT),0)
        EXTRA_CFLAGS1 += -D gcdBANK_CHANNEL_BIT=$(BANK_CHANNEL_BIT)
    endif
endif

ifeq ($(FPGA_BUILD),1)
EXTRA_CFLAGS1 += -D gcdFPGA_BUILD=1
else
EXTRA_CFLAGS1 += -D gcdFPGA_BUILD=0
endif

ifeq ($(SECURITY),1)
EXTRA_CFLAGS1 += -D gcdSECURITY=1
endif

ifeq ($(VIVANTE_ENABLE_DRM),1)
EXTRA_CFLAGS1 += -D gcdENABLE_DRM=1
else
EXTRA_CFLAGS1 += -D gcdENABLE_DRM=0
endif
EXTRA_CFLAGS1 += -D gcdLINUX_SYNC_FILE=1

EXTRA_INCLUDE := -I$(KERNEL_SRC)/$(M)/hal/inc
EXTRA_INCLUDE += -I$(KERNEL_SRC)/$(M)/hal/kernel/inc
EXTRA_INCLUDE += -I$(KERNEL_SRC)/$(M)/hal/kernel
EXTRA_INCLUDE += -I$(KERNEL_SRC)/$(M)/hal/kernel/arch
EXTRA_INCLUDE += -I$(KERNEL_SRC)/$(M)/hal/os/linux/kernel
EXTRA_INCLUDE += -I$(KERNEL_SRC)/$(M)/hal/os/linux/kernel/allocator/default/
EXTRA_INCLUDE += -I$(KERNEL_SRC)/$(M)/hal/security_v1/
EXTRA_INCLUDE += -Idrivers/staging/android
EXTRA_INCLUDE += -Iarch/arm/mm

CONFIGS_BUILD := -Wno-undef -Wno-pointer-sign \
		-Wno-unused-const-variable \
		-Wimplicit-function-declaration \

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/hal  modules ARCH=$(KERNEL_ARCH)  "EXTRA_CFLAGS+=-I$(INCLUDE) -Wno-error -I$(EXTRA_CFLAGS1) $(CONFIGS_BUILD) $(EXTRA_INCLUDE)" $(CONFIGS)

all:modules

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 M=$(M)/hal -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name "*.ko" -exec cp {} ${OUT_DIR}/../vendor_lib/modules/ \;

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean

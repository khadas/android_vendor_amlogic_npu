##############################################################################
#
#    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(PLATFORM_VENDOR),1)
LOCAL_VENDOR_MODULE  := true
endif
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../sdk/inc \
		    $(LOCAL_PATH)/../../applib/nnapi \
		    $(LOCAL_PATH)/../../applib/ovxinc/include \
		    $(LOCAL_PATH)/nnrt \
		    $(LOCAL_PATH)/nnrt \
		    $(LOCAL_PATH)/../../applib/ \
		    $(LOCAL_PATH)/extension_op/ \
		    $(LOCAL_PATH)/nnrt/boost/libs/preprocessor/include \
		    $(LOCAL_PATH)/../../../../../../frameworks/ml/nn/common/include/ \
		    $(LOCAL_PATH)/../../../../../../frameworks/ml/nn/runtime/include/    \
                    $(LOCAL_PATH)/../../../../../../external/boringssl/src/include

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libdl   \
    libhardware \
    libhidlbase \
    libhidlmemory   \
    libhidltransport    \
    liblog  \
    libutils    \
    libcutils    \
    android.hardware.neuralnetworks@1.0 \
    android.hidl.allocator@1.0  \
    android.hidl.memory@1.0 \
    libneuralnetworks   \
    libovxlib\
    libnnrt\
    libcrypto

LOCAL_SRC_FILES:= \
    VsiRTInfo.cpp \
    1.0/VsiDevice1_0.cpp \
    SandBox.cpp

ifeq ($(PRODUCT_CHIP_ID), PID0x99)
LOCAL_SRC_FILES += 1.0/VsiDriver1_0_99.cpp
LOCAL_SRC_FILES += VsiDriver_99.cpp
LOCAL_SRC_FILES += VsiPreparedModel_99.cpp
$(info 'enter 99')
else
LOCAL_SRC_FILES += 1.0/VsiDriver1_0.cpp
LOCAL_SRC_FILES += VsiDriver.cpp
LOCAL_SRC_FILES += VsiPreparedModel.cpp
$(info 'enter other')
endif


ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 28),1)
LOCAL_SRC_FILES += 1.1/VsiDevice1_1.cpp
ifeq ($(PRODUCT_CHIP_ID), PID0x99)
LOCAL_SRC_FILES += 1.1/VsiDriver1_1_99.cpp
else
LOCAL_SRC_FILES += 1.1/VsiDriver1_1.cpp
endif
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_SRC_FILES += \
    1.2/VsiDevice1_2.cpp\
    1.2/VsiPreparedModel1_2.cpp\
    1.2/VsiDriver1_2.cpp \
    1.2/VsiBurstExecutor.cpp    \
    hal_limitation/nnapi_limitation.cpp \
    hal_limitation/support.cpp
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 30),1)
LOCAL_SRC_FILES += \
    1.3/VsiDevice1_3.cpp \
    1.3/VsiDriver1_3.cpp \
    1.3/VsiPrepareModel1_3.cpp \
    1.3/VsiBuffer.cpp
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 28),1)
LOCAL_SHARED_LIBRARIES += android.hardware.neuralnetworks@1.1
LOCAL_STATIC_LIBRARIES += libneuralnetworks_common

LOCAL_CFLAGS += -Wno-error=unused-variable -Wno-error=unused-function -Wno-error=return-type \
                -Wno-unused-parameter

LOCAL_C_INCLUDES += frameworks/native/libs/nativewindow/include \
                    frameworks/native/libs/arect/include
LOCAL_SHARED_LIBRARIES += libneuralnetworks

LOCAL_MODULE      := android.hardware.neuralnetworks@1.1-service-ovx-driver
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.1-service-ovx-driver.rc

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_C_INCLUDES += frameworks/ml/nn/runtime/include \
                    frameworks/native/libs/ui/include \
                    frameworks/native/libs/nativebase/include \
                    system/libfmq/include   \
                    $(LOCAL_PATH)/hal_limitation \
                    $(LOCAL_PATH)/op_validate \
                    $(LOCAL_PATH)/extension_op

LOCAL_SHARED_LIBRARIES += libfmq \
                          libui \
                          android.hardware.neuralnetworks@1.2

LOCAL_MODULE      := android.hardware.neuralnetworks@1.2-service-ovx-driver
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.2-service-ovx-driver.rc

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 30),1)
LOCAL_SHARED_LIBRARIES += \
                          android.hardware.neuralnetworks@1.3 \
                          libnativewindow
LOCAL_CFLAGS += -DANDROID_NN_API=30

LOCAL_MODULE      := android.hardware.neuralnetworks@1.3-service-ovx-driver
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.3-service-ovx-driver.rc

endif  #30

endif  #29

endif  #28

LOCAL_CFLAGS += -DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)  -Wno-error=unused-parameter
LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin/hw
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif

LOCAL_MODULE_RELATIVE_PATH := hw
#LOCAL_INIT_RC := android.hardware.neuralnetworks@1.1-service-ovx-driver.rc

LOCAL_CFLAGS += -DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)  -Wno-error=unused-parameter\
                -Wno-unused-private-field \
                -Wno-unused-parameter \
                -Wno-delete-non-virtual-dtor -Wno-non-virtual-dtor\

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

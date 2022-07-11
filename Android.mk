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
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH), arm64)
LIB_PATH=libraryso/lib64/so_p
Target=lib64
else
LIB_PATH=libraryso/lib32/so_p
Target=lib
endif

RRODUCT_PATH = $(LIB_PATH)/PID0x88
ifeq ($(PRODUCT_CHIP_ID), PID0x88)
RRODUCT_PATH := $(LIB_PATH)/PID0x88
endif

ifeq ($(PRODUCT_CHIP_ID), PID0x99)
RRODUCT_PATH := $(LIB_PATH)/PID0x99
endif

ifeq ($(PRODUCT_CHIP_ID), PID0xB9)
RRODUCT_PATH := $(LIB_PATH)/PID0xB9
endif

ifeq ($(PRODUCT_CHIP_ID), PID0xE8)
RRODUCT_PATH := $(LIB_PATH)/PID0xE8
endif



include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(RRODUCT_PATH)/libOvx12VXCBinary.so
LOCAL_MODULE         := libOvx12VXCBinary
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
LOCAL_CHECK_ELF_FILES := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/$(Target)
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(RRODUCT_PATH)/libNNVXCBinary.so
LOCAL_MODULE         := libNNVXCBinary
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
LOCAL_CHECK_ELF_FILES := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/$(Target)
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(RRODUCT_PATH)/libNNGPUBinary.so
LOCAL_MODULE         := libNNGPUBinary
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
LOCAL_CHECK_ELF_FILES := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/$(Target)
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(LIB_PATH)/libCLC.so
LOCAL_MODULE         := libCLC
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
LOCAL_CHECK_ELF_FILES := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/$(Target)
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(LIB_PATH)/libOpenCL.so
LOCAL_MODULE         := libOpenCL
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
LOCAL_CHECK_ELF_FILES := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/$(Target)
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)



include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(LIB_PATH)/libnnrt.so
LOCAL_MODULE         := libnnrt
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
LOCAL_CHECK_ELF_FILES := false
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/$(Target)
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

ifeq ($(BOARD_NPU_SERVICE_ENABLE), true)
include $(LOCAL_PATH)/service/Android.mk
endif


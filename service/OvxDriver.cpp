/****************************************************************************
*
*    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "OvxDriver"

#include "OvxDevice.h"
#include "OvxExecutor.h"

#if ANDROID_SDK_VERSION > 27
#include "ValidateHal.h"
#endif

#include "HalInterfaces.h"
#include "Utils.h"

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

namespace android {
namespace nn {
namespace ovx_driver {

class OvxDriver : public OvxDevice {
public:
    OvxDriver() : OvxDevice("ovx-driver") {}
    ~OvxDriver() override {}

#if ANDROID_SDK_VERSION < 28
    Return<void> getCapabilities(getCapabilities_cb _hidl_cb) override;
    Return<void> getSupportedOperations(const Model& model, getSupportedOperations_cb cb) override;
#else

    Return<void> getCapabilities_1_1(getCapabilities_1_1_cb _hidl_cb) override;

    Return<void> getSupportedOperations_1_1(
        const V1_1::Model& model, getSupportedOperations_1_1_cb cb) override;
#endif
};
#if ANDROID_SDK_VERSION < 28

Return<void> OvxDriver::getCapabilities(getCapabilities_cb cb) {
#else
Return<void> OvxDriver::getCapabilities_1_1(getCapabilities_1_1_cb cb) {
#endif
    android::nn::initVLogMask();
    VLOG(DRIVER) << "getCapabilities()";
    Capabilities capabilities = {.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f},
                                 .quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f},
                                 .relaxedFloat32toFloat16Performance = {.execTime = 0.9f, .powerUsage = 0.9f}
                                };
    cb(ErrorStatus::NONE, capabilities);
    return Void();
}
#if ANDROID_SDK_VERSION < 28

Return<void> OvxDriver::getSupportedOperations(const Model& model,
                                                     getSupportedOperations_cb cb) {
#else
Return<void> OvxDriver::getSupportedOperations_1_1(
    const V1_1::Model& model, getSupportedOperations_1_1_cb cb) {
#endif
    VLOG(DRIVER) << "getSupportedOperations()";
    if (validateModel(model)) {
        const size_t count = model.operations.size();
        std::vector<bool> supported(count, true);
        cb(ErrorStatus::NONE, supported);
    } else {
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
    }
    return Void();
}

} // namespace ovx_driver
} // namespace nn
} // namespace android

using android::nn::ovx_driver::OvxDriver;
using android::sp;

int main() {
    sp<OvxDriver> driver(new OvxDriver());
    return driver->run();
}

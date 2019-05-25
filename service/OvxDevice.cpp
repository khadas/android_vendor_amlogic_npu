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

#define LOG_TAG "OvxDevice"

#include "OvxDevice.h"

//#include "CpuExecutor.h"
#include "HalInterfaces.h"

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

namespace android {
namespace nn {
namespace ovx_driver {

#if ANDROID_SDK_VERSION < 28
Return<ErrorStatus> OvxDevice::prepareModel(const Model& model,
#else

Return<ErrorStatus> OvxDevice::prepareModel(const V1_0::Model& model,
                                               const sp<IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        VLOG(DRIVER) << "prepareModel";
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model)) {
        VLOG(DRIVER) << "prepareModel";
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }
    return prepareModel_1_1(convertToV1_1(model), ExecutionPreference::FAST_SINGLE_ANSWER,
                            callback);
}

Return<ErrorStatus> OvxDevice::prepareModel_1_1(
                                               const V1_1::Model& model,
                                               ExecutionPreference /*preference*/,
#endif
                                               const sp<IPreparedModelCallback>& callback) {
    if (VLOG_IS_ON(DRIVER)) {
        logModelToInfo(model);
    }
    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    // TODO: make asynchronous later
    sp<OvxPreparedModel> preparedModel = new OvxPreparedModel(model);
    if (!preparedModel->initialize(&mContext, &mMutex)) {
       callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
       return ErrorStatus::INVALID_ARGUMENT;
    }
    callback->notify(ErrorStatus::NONE, preparedModel);
    return ErrorStatus::NONE;
}

Return<DeviceStatus> OvxDevice::getStatus() {
    return DeviceStatus::AVAILABLE;
}

#if ANDROID_SDK_VERSION > 27

Return<void> OvxDevice::getSupportedOperations(const V1_0::Model& model,
                                               getSupportedOperations_cb cb) {
    if (!validateModel(model)) {
        VLOG(DRIVER) << "getSupportedOperations";
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }

    return getSupportedOperations_1_1(convertToV1_1(model), cb);
}


Return<void> OvxDevice::getCapabilities(getCapabilities_cb cb) {
    return getCapabilities_1_1(
        [&](ErrorStatus error, const V1_1::Capabilities& capabilities) {
            // TODO(dgross): Do we need to check compliantWithV1_0(capabilities)?
            cb(error, convertToV1_0(capabilities));
        });
}

#endif

int OvxDevice::run() {
    android::hardware::configureRpcThreadpool(4, true);
    if (registerAsService(mName) != android::OK) {
        LOG(ERROR) << "Could not register service";
        return 1;
    }
    android::hardware::joinRpcThreadpool();
    return 1;
}

bool OvxPreparedModel::initialize(vx_context* context, pthread_mutex_t* mutex) {

    mExecutor = new OvxExecutor();

    mExecutor->initalize(context, mutex, &mModel, &mPoolInfos);

    return true;
}

void OvxPreparedModel::asyncExecute(const Request& request,
                                       const sp<IExecutionCallback>& callback) {
    std::vector<VxRunTimePoolInfo> requestPoolInfos;
    if (!setRunTimePoolInfosFromHidlMemories(&requestPoolInfos, request.pools)) {
        callback->notify(ErrorStatus::GENERAL_FAILURE);
        return;
    }

    int n = mExecutor->run(mModel, request, requestPoolInfos);
    ErrorStatus executionStatus =
            n == ANEURALNETWORKS_NO_ERROR ? ErrorStatus::NONE : ErrorStatus::GENERAL_FAILURE;
    Return<void> returned = callback->notify(executionStatus);
    if (!returned.isOk()) {
        LOG(ERROR) << " hidl callback failed to return properly: " << returned.description();
    }
}

Return<ErrorStatus> OvxPreparedModel::execute(const Request& request,
                                                 const sp<IExecutionCallback>& callback) {
    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to execute";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateRequest(request, mModel)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    // This thread is intentionally detached because the sample driver service
    // is expected to live forever.
    std::thread([this, request, callback]{ asyncExecute(request, callback); }).detach();

    return ErrorStatus::NONE;
}

} // namespace ovx_driver
} // namespace nn
} // namespace android

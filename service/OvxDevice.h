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

#ifndef ANDROID_ML_NN_SAMPLE_DRIVER_SAMPLE_DRIVER_H
#define ANDROID_ML_NN_SAMPLE_DRIVER_SAMPLE_DRIVER_H

#include "OvxExecutor.h"
#include "HalInterfaces.h"
#include "NeuralNetworks.h"

#if ANDROID_SDK_VERSION > 27
#include "ValidateHal.h"
#endif
#include <pthread.h>

#include "VX/vx.h"

#include <string>
#include <sys/system_properties.h>

using android::sp;

namespace android {
namespace nn {
namespace ovx_driver {

// Base class used to create sample drivers for the NN HAL.  This class
// provides some implementation of the more common functions.
//
// Since these drivers simulate hardware, they must run the computations
// on the CPU.  An actual driver would not do that.
class OvxDevice : public IDevice {
public:
    OvxDevice(const char* name) : mName(name) {
#ifndef MULTI_CONTEXT
        mContext = vxCreateContext();
#endif
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);

        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

        pthread_mutex_init(&mMutex, &attr);

        pthread_mutexattr_destroy(&attr);
    }
    ~OvxDevice() override {

        pthread_mutex_lock(&mMutex);

        if (mContext != nullptr)
        {
            vxReleaseContext(&mContext);
        }

        pthread_mutex_unlock(&mMutex);

        pthread_mutex_destroy(&mMutex);
    }
#if ANDROID_SDK_VERSION < 28
    Return<ErrorStatus> prepareModel(const Model& model,
#else


    Return<void> getCapabilities(getCapabilities_cb _hidl_cb) override;

    Return<void> getSupportedOperations(const V1_0::Model& model,
        getSupportedOperations_cb cb) override;

    Return<ErrorStatus> prepareModel(
        const V1_0::Model& model,
        const sp<IPreparedModelCallback>& callback) override;
    Return<ErrorStatus> prepareModel_1_1(
        const V1_1::Model& model,
        ExecutionPreference preference,

#endif
                                     const sp<IPreparedModelCallback>& callback) override;
    Return<DeviceStatus> getStatus() override;

    // Starts and runs the driver service.  Typically called from main().
    // This will return only once the service shuts down.
    int run();
protected:
    vx_context mContext = nullptr;

    pthread_mutex_t mMutex;

    std::string mName;
};

class OvxPreparedModel : public IPreparedModel {
public:
    OvxPreparedModel(const Model& model)
          : // Make a copy of the model, as we need to preserve it.
            mModel(model) {}
    ~OvxPreparedModel() override {mExecutor = nullptr;}
    bool initialize(vx_context* context, pthread_mutex_t* mutex, vx_bool async);
    Return<ErrorStatus> execute(const Request& request,
                                const sp<IExecutionCallback>& callback) override;

private:
    void asyncExecute(const Request& request, const sp<IExecutionCallback>& callback);

    void env();

    Model mModel;
    std::vector<VxRunTimePoolInfo> mPoolInfos;
    sp<OvxExecutor> mExecutor = nullptr;

    vx_bool mAsyncMode = vx_true_e;
};

} // namespace ovx_driver
} // namespace nn
} // namespace android

#endif // ANDROID_ML_NN_SAMPLE_DRIVER_SAMPLE_DRIVER_H

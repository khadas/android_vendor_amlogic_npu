/****************************************************************************
*
*    Copyright (c) 2019 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#define LOG_TAG "VsiDevice"

#include "VsiDevice.h"
#include "VsiPreparedModel.h"

#include "HalInterfaces.h"

#if ANDROID_SDK_VERSION > 27
#include "ValidateHal.h"
#endif

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

namespace android {
namespace nn {
namespace vsi_driver {

Return<ErrorStatus> VsiDevice::prepareModel_1_1(const V1_1::Model& model,
#if ANDROID_SDK_VERSION > 27
                                                ExecutionPreference preference,
#endif
                                                const sp<V1_0::IPreparedModelCallback>& callback) {
    if (VLOG_IS_ON(DRIVER)) {
        VLOG(DRIVER) << "prepareModel";
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
    sp<VsiPreparedModel> preparedModel = new VsiPreparedModel(model);
    if (!preparedModel.get()) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }
    Return<void> returned = callback->notify(ErrorStatus::NONE, preparedModel);
    if (!returned.isOk()) {
        LOG(ERROR) << " hidl callback failed to return properly: " << returned.description();
    }
    return ErrorStatus::NONE;
}

Return<DeviceStatus> VsiDevice::getStatus() {
    VLOG(DRIVER) << "getStatus()";
    return DeviceStatus::AVAILABLE;
}

int VsiDevice::run() {
    // TODO: Increase ThreadPool to 4 ?
    android::hardware::configureRpcThreadpool(1, true);
    if (registerAsService(name_) != android::OK) {
        LOG(ERROR) << "Could not register service";
        return 1;
    }
    android::hardware::joinRpcThreadpool();

    return 1;
}

}  // namespace ovx_driver
}  // namespace nn
}  // namespace android

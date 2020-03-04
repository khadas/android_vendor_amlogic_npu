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

#include "VsiDevice1_2.h"
#include "VsiPreparedModel1_2.h"

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
    static void notify(const sp<V1_0::IPreparedModelCallback>& callback, const ErrorStatus& status,
                       const sp<VsiPreparedModel>& preparedModel) {
        callback->notify(status, preparedModel);
    }

    static void notify(const sp<V1_2::IPreparedModelCallback>& callback, const ErrorStatus& status,
                       const sp<VsiPreparedModel>& preparedModel) {
        callback->notify_1_2(status, preparedModel);
    }

    template <typename T_IPreparedModelCallback>
    void asyncPrepareModel(sp<VsiPreparedModel> vsiPreapareModel, const T_IPreparedModelCallback &callback){
          auto status = vsiPreapareModel->initialize();
          if( ErrorStatus::NONE != status){
            notify(callback, status, nullptr);
            return;
          }
          notify(callback, status, vsiPreapareModel);
    }

    template <typename T_Model, typename T_IPreparedModelCallback>
    Return<ErrorStatus> VsiDevice::prepareModelBase(const T_Model& model,
                                         ExecutionPreference preference,
                                         const sp<T_IPreparedModelCallback>& callback) {
        if (VLOG_IS_ON(DRIVER)) {
            VLOG(DRIVER) << "prepareModel";
            logModelToInfo(model);
        }
        if (callback.get() == nullptr) {
            LOG(ERROR) << "invalid callback passed to prepareModel";
            return ErrorStatus::INVALID_ARGUMENT;
        }
        if (!validateModel(model)) {
            LOG(ERROR) << "invalid hal model";
            notify(callback, ErrorStatus::INVALID_ARGUMENT, nullptr);
            return ErrorStatus::INVALID_ARGUMENT;
        }
        if( !validateExecutionPreference(preference)){
            LOG(ERROR) << "invalid preference" << static_cast<int32_t>(preference);
            notify(callback, ErrorStatus::INVALID_ARGUMENT, nullptr);
            return ErrorStatus::INVALID_ARGUMENT;
        }

        sp<VsiPreparedModel> preparedModel = new VsiPreparedModel( convertToV1_2(model));
        std::thread(asyncPrepareModel<sp<T_IPreparedModelCallback>>, preparedModel, callback).detach();

        return ErrorStatus::NONE;

    }

    Return<ErrorStatus> VsiDevice::prepareModel(const V1_0::Model& model,
                                                const sp<V1_0::IPreparedModelCallback>& callback) {
        return prepareModelBase(model, ExecutionPreference::FAST_SINGLE_ANSWER, callback);
    }

    Return<ErrorStatus> VsiDevice::prepareModel_1_1(
        const V1_1::Model& model,
        ExecutionPreference preference,
        const sp<V1_0::IPreparedModelCallback>& callback)  {
        return prepareModelBase(model, preference, callback);
    }

    Return<ErrorStatus> VsiDevice::prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
        const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache,
        const HidlToken& token,
        const sp<V1_2::IPreparedModelCallback>& callback) {
        return prepareModelBase(model, preference, callback);
    }

    Return<ErrorStatus> VsiDevice::prepareModelFromCache(
        const hidl_vec<hidl_handle>&, const hidl_vec<hidl_handle>&, const HidlToken&,
        const sp<V1_2::IPreparedModelCallback>& callback) {
        notify(callback, ErrorStatus::GENERAL_FAILURE, nullptr);
        return ErrorStatus::GENERAL_FAILURE;
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
}
}
}


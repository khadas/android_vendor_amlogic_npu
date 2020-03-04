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

#ifndef ANDROID_ML_NN_VSI_DEVICE_1_2_H
#define ANDROID_ML_NN_VSI_DEVICE_1_2_H

#include "HalInterfaces.h"
#include "model.hpp"
#include "Utils.h"
#include "Tracing.h"
#include "ExecutionBurstServer.h"

#if ANDROID_SDK_VERSION > 27
#include "ValidateHal.h"
#endif
#include <pthread.h>

#include <string>

using android::sp;
using HidlToken = hidl_array<uint8_t, ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN>;

namespace android {
namespace nn {
namespace vsi_driver {


class VsiDevice : public V1_2::IDevice {
   public:
    VsiDevice(const char* name) : name_(name) {}
    ~VsiDevice() override {}

    Return<ErrorStatus> prepareModel(
        const V1_0::Model& model,
        const sp<V1_0::IPreparedModelCallback>& callback) override;

    Return<ErrorStatus> prepareModel_1_1(
        const V1_1::Model& model,
        ExecutionPreference preference,
        const sp<V1_0::IPreparedModelCallback>& callback) override;

    Return<ErrorStatus> prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
        const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache,
        const HidlToken& token,
        const sp<V1_2::IPreparedModelCallback>& callback) override;

    Return<ErrorStatus> prepareModelFromCache(
        const hidl_vec<hidl_handle>& modelCache, const hidl_vec<hidl_handle>& dataCache,
        const HidlToken& token, const sp<V1_2::IPreparedModelCallback>& callback) override;

    Return<DeviceStatus> getStatus() override;

    bool Initialize() {
        run();
        return true;
    }

    // Device driver entry_point
    virtual int run();

   protected:
    std::string name_;
   private:
        template <typename T_Model, typename T_IPreparedModelCallback>
        Return<ErrorStatus> prepareModelBase(const T_Model& model,
                                         ExecutionPreference ,
                                         const sp<T_IPreparedModelCallback>& callback);
};
}
}
}

#endif

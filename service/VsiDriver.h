/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
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

#ifndef ANDROID_ML_NN_VSI_DRIVER_H
#define ANDROID_ML_NN_VSI_DRIVER_H

#include "VsiDevice.h"
#include "HalInterfaces.h"
#include "Utils.h"
#include "CustomizeOpSupportList.h"

#include <sys/system_properties.h>
#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>


namespace android {
namespace nn {
namespace vsi_driver {

class VsiDriver : public VsiDevice {
   public:
    VsiDriver() : VsiDevice("ovx-driver") {initalizeEnv();}
    Return<void> getCapabilities(getCapabilities_cb _hidl_cb) ;
    Return<void> getSupportedOperations(const V1_0::Model& model, V1_0::IDevice::getSupportedOperations_cb cb) ;

#if ANDROID_SDK_VERSION >= 28
    Return<void> getCapabilities_1_1(V1_1::IDevice::getCapabilities_1_1_cb _hidl_cb) ;
    Return<void> getSupportedOperations_1_1(const V1_1::Model& model,
                                                  V1_1::IDevice::getSupportedOperations_1_1_cb cb) ;
#endif

#if ANDROID_SDK_VERSION >= 29
    Return<void> getSupportedExtensions(V1_2::IDevice::getSupportedExtensions_cb _hidl_cb);
    Return<void> getCapabilities_1_2(V1_2::IDevice::getCapabilities_1_2_cb _hidl_cb) ;
    Return<void> getSupportedOperations_1_2( const V1_2::Model& model,
                                                   V1_2::IDevice::getSupportedOperations_1_2_cb cb) ;

    static uint16_t findExtensionOperation(const HalPlatform::Operation& operation) {
        int32_t operationType = static_cast<int32_t>(operation.type);
        const uint8_t kLowBitsType = static_cast<uint8_t>(HalPlatform::Model::ExtensionTypeEncoding::LOW_BITS_TYPE);
        const uint32_t kTypeWithinExtensionMask = (1 << kLowBitsType) - 1;
        uint16_t extensionSupportMask = static_cast<int32_t>(operationType) >> kLowBitsType;
        uint16_t typeWithinExtension = static_cast<int32_t>(operationType) & kTypeWithinExtensionMask;
        if (extensionSupportMask != 0) return typeWithinExtension;
        return 0;
    };
#endif

    static bool isSupportedOperation(const HalPlatform::Operation& operation,
                                     const HalPlatform::Model& model,
                                     std::string& not_support_reason);

    static const uint8_t* getOperandDataPtr( const HalPlatform::Model& model,
                                             const HalPlatform::Operand& hal_operand,
                                             VsiRTInfo &vsiMemory);

   private:
   int32_t disable_float_feature_; // switch that float-type running on hal
   private:
    void initalizeEnv();

    template <typename T_model, typename T_getSupportOperationsCallback>
    Return<void> getSupportedOperationsBase(const T_model& model,
                                            T_getSupportOperationsCallback cb){
        LOG(INFO) << "getSupportedOperations";
        if (validateModel(model)) {
            const size_t count = model.operations.size();
            std::vector<bool> supported(count, true);
            std::string notSupportReason = "";
            for (size_t i = 0; i < count; i++) {
                const auto& operation = model.operations[i];

#if ANDROID_SDK_VERSION >= 29
                supported[i] = findExtensionOperation(operation) ||
                               (!IsOpBlocked(static_cast<int32_t>(operation.type)) &&
                                isSupportedOperation(operation, model, notSupportReason));
#else
                supported[i] = !IsOpBlocked(static_cast<int32_t>(operation.type)) &&
                               isSupportedOperation(operation, model, notSupportReason);
#endif
            }
            LOG(INFO) << notSupportReason;
            cb(ErrorStatus::NONE, supported);
        } else {
            LOG(ERROR) << "invalid model";
            std::vector<bool> supported;
            cb(ErrorStatus::INVALID_ARGUMENT, supported);
        }
        LOG(INFO) << "getSupportedOperations exit";
        return Void();
    };

};

}
}
}
#endif

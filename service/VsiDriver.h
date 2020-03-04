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

#ifndef ANDROID_ML_NN_VSI_DRIVER_H
#define ANDROID_ML_NN_VSI_DRIVER_H

#if ANDROID_SDK_VERSION > 28
#include "VsiDevice1_2.h"
#include <ui/GraphicBuffer.h>
#elif ANDROID_SDK_VERSION > 27
#include "VsiDevice.h"
#endif

#include "HalInterfaces.h"
#include "Utils.h"

#if ANDROID_SDK_VERSION > 27
#include "ValidateHal.h"
#endif

#include <sys/system_properties.h>
#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>


namespace android {
namespace nn {
namespace vsi_driver {

    /*record the info that is gotten from hidl_memory*/
    struct VsiRTInfo{
        sp<IMemory>         shared_mem;             /* if hidl_memory is "ashmem", */
                                                    /* the shared_mem is relative to ptr */
        size_t                buffer_size;
        std::string         mem_type;               /* record type of hidl_memory*/
        uint8_t *           ptr;                    /* record the data pointer gotten from "ashmem" hidl_memory*/
        std::shared_ptr<nnrt::Memory>  vsi_mem;   /* ovx memory object converted from "mmap_fd" hidl_memory*/
#if ANDROID_SDK_VERSION > 28
        sp<GraphicBuffer> graphic_buffer;
#endif

    VsiRTInfo(): buffer_size(0), ptr(nullptr)
#if ANDROID_SDK_VERSION > 28
    ,graphic_buffer(nullptr)
#endif
        {};

    ~VsiRTInfo(){
        if("mmap_fd" == mem_type){
            if(vsi_mem)
                vsi_mem.reset();
        }
#if ANDROID_SDK_VERSION > 28
        else if("hardware_buffer_blob" == mem_type){
            if(graphic_buffer){
                graphic_buffer->unlock();
                graphic_buffer = nullptr;
            }
        }
#endif
        };
    };

class VsiDriver : public VsiDevice {
   public:
    VsiDriver() : VsiDevice("ovx-driver") {initalizeEnv();}
    Return<void> getCapabilities(getCapabilities_cb _hidl_cb) ;
    Return<void> getSupportedOperations(const V1_0::Model& model, V1_0::IDevice::getSupportedOperations_cb cb) ;

#if ANDROID_SDK_VERSION > 27
    Return<void> getCapabilities_1_1(V1_1::IDevice::getCapabilities_1_1_cb _hidl_cb) ;
    Return<void> getSupportedOperations_1_1(const V1_1::Model& model,
                                                                V1_1::IDevice::getSupportedOperations_1_1_cb cb) ;
#endif

#if ANDROID_SDK_VERSION > 28
    Return<void> getCapabilities_1_2(V1_2::IDevice::getCapabilities_1_2_cb _hidl_cb) ;
    Return<void> getSupportedOperations_1_2(const V1_2::Model& model,
                                            V1_2::IDevice::getSupportedOperations_1_2_cb cb) ;

    Return<void>
        getVersionString(V1_2::IDevice::getVersionString_cb _hidl_cb) {
        _hidl_cb(ErrorStatus::NONE, "android hal vsi npu 1.2 alpha");
        return Void();
    };

    Return<void>
        getType(V1_2::IDevice::getType_cb _hidl_cb) {
        _hidl_cb(ErrorStatus::NONE, V1_2::DeviceType::ACCELERATOR);
        return Void();
    };

    Return<void>
        getSupportedExtensions(V1_2::IDevice::getSupportedExtensions_cb _hidl_cb) {
        _hidl_cb(ErrorStatus::NONE, {/* No extensions. */});
        return Void();
    };

    Return<void>
        getNumberOfCacheFilesNeeded(V1_2::IDevice::getNumberOfCacheFilesNeeded_cb _hidl_cb) {
        // Set both numbers to be 0 for cache not supported.
        _hidl_cb(ErrorStatus::NONE, /*numModelCache=*/0, /*numDataCache=*/0);
        return Void();
    };
#endif

    template<typename T_operation,typename T_Model>
    static bool isSupportedOperation(const T_operation &operation, const T_Model& model);

   static bool mapHidlMem(const hidl_memory & hidl_memory, VsiRTInfo &vsiMemory);

   private:
   int32_t disable_float_feature_; // switch that float-type running on hal
   private:
    void initalizeEnv();

    template <typename T_model, typename T_getSupportOperationsCallback>
    Return<void> getSupportedOperationsBase(const T_model& model,
                                            T_getSupportOperationsCallback cb);

    template<typename T_Model>
    static const uint8_t* getOperandDataPtr(const T_Model &model, const Operand& hal_operand, VsiRTInfo &vsiMemory);
};

}
}
}
#endif

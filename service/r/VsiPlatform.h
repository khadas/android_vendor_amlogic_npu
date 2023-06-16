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
#ifndef ANDROID_ML_NN_VSI_PALTFORM_H
#define ANDROID_ML_NN_VSI_PALTFORM_H

#include <nnrt/file_map_memory.hpp>

#include "HalInterfaces.h"
#include "Utils.h"

#if ANDROID_SDK_VERSION >= 28
#include "ValidateHal.h"
#endif

#if ANDROID_SDK_VERSION >= 29
#include <ui/GraphicBuffer.h>

#include "ExecutionBurstServer.h"
#include "Tracing.h"
#endif

/*alias namespace for Model*/
#if ANDROID_SDK_VERSION >= 29
using android::hardware::hidl_array;
using android::hardware::hidl_memory;
using android::hidl::memory::V1_0::IMemory;
#endif

#if ANDROID_SDK_VERSION >= 29
using android::sp;
using HidlToken = hidl_array<uint8_t, ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN>;
#endif

namespace android {
namespace nn {
namespace hal {}
namespace vsi_driver {
using namespace hal;

template <int ANDROID_VERSION>
struct Hal {};

template <>
struct Hal<27> {
    using Device = V1_0::IDevice;
    using PrepareModel = V1_0::IPreparedModel;
    using Operand = V1_0::Operand;
    using OperandType = V1_0::OperandType;
    using Operation = V1_0::Operation;
    using OperationType = V1_0::OperationType;
    using Model = V1_0::Model;
    using OperandLifeTime = V1_0::OperandLifeTime;
    using ErrorStatus = V1_0::ErrorStatus;
    using Request = V1_0::Request;

    template <typename T_type>
    static inline T_type convertVersion(const T_type& variable) {
        return variable;
    }

    template <typename T_type>
    static bool inline validatePool(const T_type& hidl_pool) {
        return true;
    }
};

#if ANDROID_SDK_VERSION >= 28
template <>
struct Hal<28> {
    using Device = V1_1::IDevice;
    using PrepareModel = V1_0::IPreparedModel;
    using Operand = V1_0::Operand;
    using OperandType = V1_0::OperandType;
    using Operation = V1_1::Operation;
    using OperationType = V1_1::OperationType;
    using Model = V1_1::Model;
    using OperandLifeTime = V1_0::OperandLifeTime;
    using ErrorStatus = V1_0::ErrorStatus;
    using Request = V1_0::Request;

    template <typename T_type>
    static auto inline convertVersion(T_type variable) {
        return android::nn::convertToV1_1(variable);
    }

    template <typename T_type>
    static bool inline validatePool(const T_type& hidl_pool) {
        return true;
    }
};
#endif

#if ANDROID_SDK_VERSION >= 29
template <>
struct Hal<29> {
    using Device = V1_2::IDevice;
    using PrepareModel = V1_2::IPreparedModel;
    using Operand = V1_2::Operand;
    using OperandType = V1_2::OperandType;
    using Operation = V1_2::Operation;
    using OperationType = V1_2::OperationType;
    using Model = V1_2::Model;
    using OperandLifeTime = V1_0::OperandLifeTime;
    using ErrorStatus = V1_0::ErrorStatus;
    using Request = V1_0::Request;

    template <typename T_type>
    static auto inline convertVersion(const T_type& variable) {
        return android::nn::convertToV1_2(variable);
    }

    template <typename T_type>
    static bool inline validatePool(const T_type& hidl_pool) {
        return android::nn::validatePool(hidl_pool);
        ;
    }
};
#endif

#if ANDROID_SDK_VERSION >= 30
template <>
struct Hal<30> {
    using Device = V1_3::IDevice;
    using PrepareModel = V1_3::IPreparedModel;
    using Operand = V1_3::Operand;
    using OperandType = V1_3::OperandType;
    using Operation = V1_3::Operation;
    using OperationType = V1_3::OperationType;
    using Model = V1_3::Model;
    using OperandLifeTime = V1_3::OperandLifeTime;
    using ErrorStatus = V1_0::ErrorStatus;
    using Request = V1_0::Request;

    template <typename T_type>
    static auto inline convertVersion(const T_type& variable) {
        return android::nn::convertToV1_3(variable);
    }

    template <typename T_type>
    static auto inline convertToV1_0(const T_type& variable) {
        return android::nn::convertToV1_0(variable);
    }

    template <typename T_type>
    static auto inline convertToV1_2(T_type& variable) {
        return android::nn::convertToV1_2(variable);
    }

    template <typename T_type>
    static bool inline validatePool(const T_type& hidl_pool) {
        return android::nn::validatePool(hidl_pool);
    }
};
#endif

#if ANDROID_SDK_VERSION >= 31
template <>
struct Hal<31> {
    using Device = V1_3::IDevice;
    using PrepareModel = V1_3::IPreparedModel;
    using Operand = V1_3::Operand;
    using OperandType = V1_3::OperandType;
    using Operation = V1_3::Operation;
    using OperationType = V1_3::OperationType;
    using Model = V1_3::Model;
    using OperandLifeTime = V1_3::OperandLifeTime;
    using ErrorStatus = V1_0::ErrorStatus;
    using Request = V1_0::Request;

    template <typename T_type>
    static auto inline convertVersion(const T_type& variable) {
        return android::nn::convertToV1_3(variable);
    }

    template <typename T_type>
    static auto inline convertToV1_0(const T_type& variable) {
        return android::nn::convertToV1_0(variable);
    }

    template <typename T_type>
    static auto inline convertToV1_2(T_type& variable) {
        return android::nn::convertToV1_2(variable);
    }

    template <typename T_type>
    static bool inline validatePool(const T_type& hidl_pool) {
        return android::nn::validatePool(hidl_pool);
    }
};
#endif

using HalPlatform = struct Hal<ANDROID_SDK_VERSION>;
using ErrorStatus = HalPlatform::ErrorStatus;
using Request = HalPlatform::Request;
using OperandLifeTime = HalPlatform::OperandLifeTime;
using Model = HalPlatform::Model;
using OperandType = HalPlatform::OperandType;
using OperationType = HalPlatform::OperationType;

inline static auto& GetHalOperand(const Model& model, uint32_t index) {
#if ANDROID_SDK_VERSION < 30
    return model.operands[index];
#elif ANDROID_SDK_VERSION >= 30
    return model.main.operands[index];
#endif
}


#if ANDROID_SDK_VERSION >= 31
using Timing = V1_2::Timing;
using OutputShape = V1_2::OutputShape;
using IFencedExecutionCallback = V1_3::IFencedExecutionCallback;
using OptionalTimeoutDuration = V1_3::OptionalTimeoutDuration;
using MeasureTiming = V1_2::MeasureTiming;
using ExecutionPreference = V1_1::ExecutionPreference;
using OptionalTimePoint = V1_3::OptionalTimePoint;
using RequestArgument = V1_0::RequestArgument;
using Priority = V1_3::Priority;
using CacheToken = HalCacheToken;
using DeviceStatus = V1_0::DeviceStatus;
using IBuffer = V1_3::IBuffer;
using PerformanceInfo = V1_0::PerformanceInfo;

using android::hardware::hidl_vec;
using android::hardware::hidl_handle;
using android::hardware::hidl_array;
using android::hardware::Return;
using android::hardware::Void;
namespace hal = android::hardware;
#endif


/**
 * @brief Get the System Property As Int
 *
 * @param prop_name
 * @param default
 * @return int
 */
int getSystemPropertyAsInt(const char* prop_name, int default_value = 0);

/**
 * @brief Get the System Property
 *
 * @param prop_name : INPUT
 * @param value : OUTPUT
 * @return int
 */
int getSystemProperty(const char* prop_name, char* value);

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

#endif

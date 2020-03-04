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
#include "VsiDriver.h"
#include "file_map_memory.hpp"

namespace android {
namespace nn {
namespace vsi_driver {

void VsiDriver::initalizeEnv() {
    disable_float_feature_ = 0;
    char env[100] = {0};
    int ireturn = __system_property_get("DISABLE_FLOAT_FEATURE", env);
    if (ireturn) {
        disable_float_feature_ = atoi(env);
        if (disable_float_feature_) LOG(INFO) << "float-type model will not running on hal";
    }
}

template <typename T_model, typename T_getSupportOperationsCallback>
Return<void> VsiDriver::getSupportedOperationsBase(const T_model& model,
                                                   T_getSupportOperationsCallback cb) {
    LOG(INFO) << "getSupportedOperations";
    if (validateModel(model)) {
        const size_t count = model.operations.size();
        std::vector<bool> supported(count, true);
        for (size_t i = 0; i < count; i++) {
            const auto& operation = model.operations[i];
            supported[i] = isSupportedOperation(operation, model);
        }
        cb(ErrorStatus::NONE, supported);
    } else {
        LOG(ERROR) << "invalid model";
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
    }
    LOG(INFO) << "getSupportedOperations exit";
    return Void();
}

Return<void> VsiDriver::getCapabilities(getCapabilities_cb cb) {
    V1_0::Capabilities capabilities;
    if (disable_float_feature_) {
        capabilities.float32Performance = {.execTime = 1.9f, .powerUsage = 1.9f};
        capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
    } else {
        capabilities.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
        capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
    }
    cb(ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> VsiDriver::getSupportedOperations(const V1_0::Model& model,
                                               V1_0::IDevice::getSupportedOperations_cb cb) {
    if (!validateModel(model)) {
        LOG(ERROR) << __FUNCTION__;
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }
#if ANDROID_SDK_VERSION > 28
    return getSupportedOperationsBase(convertToV1_2(model), cb);
#elif ANDROID_SDK_VERSION > 27
    return getSupportedOperationsBase(convertToV1_1(model), cb);
#else
    return getSupportedOperationsBase(model, cb);
#endif
}

#if ANDROID_SDK_VERSION > 27
Return<void> VsiDriver::getCapabilities_1_1(getCapabilities_1_1_cb cb) {
    V1_1::Capabilities capabilities;
    if (disable_float_feature_) {
        capabilities.float32Performance = {.execTime = 1.9f, .powerUsage = 1.9f};
        capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
        capabilities.relaxedFloat32toFloat16Performance = {.execTime = 1.5f, .powerUsage = 1.5f};
    } else {
        capabilities.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
        capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
        capabilities.relaxedFloat32toFloat16Performance = {.execTime = 0.5f, .powerUsage = 0.5f};
    }
    cb(ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> VsiDriver::getSupportedOperations_1_1(
    const V1_1::Model& model, V1_1::IDevice::getSupportedOperations_1_1_cb cb) {
    if (!validateModel(model)) {
        LOG(ERROR) << __FUNCTION__;
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }
#if ANDROID_SDK_VERSION > 28
    return getSupportedOperationsBase(convertToV1_2(model), cb);
#else
    return getSupportedOperationsBase(model, cb);
#endif
}
#endif

bool VsiDriver::mapHidlMem(const hidl_memory& hidl_memory, VsiRTInfo& vsiMemory) {
#if ANDROID_SDK_VERSION > 28
    sp<GraphicBuffer> graphic_buffer = nullptr;
#endif
    std::shared_ptr<nnrt::Memory> vsi_mem = nullptr;
    sp<IMemory> shared_mem = nullptr;
    uint8_t* buffer = nullptr;

#if ANDROID_SDK_VERSION > 28
    if (!validatePool(hidl_memory)) {
        LOG(ERROR) << "invalid hidl memory pool";
        return false;
    }
#endif

    if ("ashmem" == hidl_memory.name()) {
        shared_mem = mapMemory(hidl_memory);
        assert(shared_mem);
        shared_mem->read();
        buffer = reinterpret_cast<uint8_t*>(static_cast<void*>(shared_mem->getPointer()));
    } else if ("mmap_fd" == hidl_memory.name()) {
        size_t size = hidl_memory.size();
        int fd = hidl_memory.handle()->data[0];
        int mode = hidl_memory.handle()->data[1];
        size_t offset =
            getSizeFromInts(hidl_memory.handle()->data[2], hidl_memory.handle()->data[3]);

        vsi_mem = std::make_shared<nnrt::Memory>();
        vsi_mem->readFromFd(size, mode, fd, offset);
    }
#if ANDROID_SDK_VERSION > 28
    else if ("hardware_buffer_blob" == hidl_memory.name()) {
        auto handle = hidl_memory.handle();
        auto format = AHARDWAREBUFFER_FORMAT_BLOB;
        auto usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
        const uint32_t width = hidl_memory.size();
        const uint32_t height = 1;  // height is always 1 for BLOB mode AHardwareBuffer.
        const uint32_t layers = 1;  // layers is always 1 for BLOB mode AHardwareBuffer.
        const uint32_t stride = hidl_memory.size();
        graphic_buffer = new GraphicBuffer(handle,
                                           GraphicBuffer::HandleWrapMethod::CLONE_HANDLE,
                                           width,
                                           height,
                                           format,
                                           layers,
                                           usage,
                                           stride);
        void* gBuffer = nullptr;
        auto status = graphic_buffer->lock(usage, &gBuffer);
        if (status != NO_ERROR) {
            LOG(ERROR) << "RunTimePoolInfo Can't lock the AHardwareBuffer.";
            return false;
        }
        buffer = static_cast<uint8_t*>(gBuffer);
    } else {
        LOG(ERROR) << "invalid hidl_memory";
        return false;
    }
#endif
    vsiMemory.shared_mem = shared_mem;
    vsiMemory.mem_type = std::string(hidl_memory.name());
    vsiMemory.ptr = buffer;
    vsiMemory.vsi_mem = vsi_mem;
    vsiMemory.buffer_size = hidl_memory.size();
#if ANDROID_SDK_VERSION > 28
    vsiMemory.graphic_buffer = graphic_buffer;
#endif
    return true;
}

template <typename T_Model>
const uint8_t* VsiDriver::getOperandDataPtr(const T_Model& model,
                                            const Operand& hal_operand,
                                            VsiRTInfo& vsiMemory) {
    if (OperandLifeTime::CONSTANT_COPY == hal_operand.lifetime) {
        return model.operandValues.data() + hal_operand.location.offset;
    } else if (OperandLifeTime::CONSTANT_REFERENCE == hal_operand.lifetime) {
        if (!mapHidlMem(model.pools[hal_operand.location.poolIndex], vsiMemory)) return nullptr;

        if ("ashmem" == vsiMemory.mem_type) {
            return vsiMemory.ptr;
        } else if ("mmap_fd" == vsiMemory.mem_type) {
            return static_cast<const uint8_t*>(
                vsiMemory.vsi_mem->data(hal_operand.location.offset));
        }
#if ANDROID_SDK_VERSION > 28
        else if ("hardware_buffer_blob" == vsiMemory.mem_type) {
            return vsiMemory.ptr;
        }
#endif
    }
    return nullptr;
}
template <typename T_operation, typename T_Model>
bool VsiDriver::isSupportedOperation(const T_operation& operation, const T_Model& model) {
#if ANDROID_SDK_VERSION > 28
    auto checkSupportedOperand = [](auto& operand) -> bool {
        bool isSupported = true;
        switch (operand.type) {
            // API 29 newly added operand
            case OperandType::BOOL:
            case OperandType::TENSOR_QUANT16_SYMM:
            case OperandType::TENSOR_FLOAT16:
            case OperandType::TENSOR_BOOL8:
            case OperandType::FLOAT16:
            case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
            case OperandType::TENSOR_QUANT16_ASYMM:
            case OperandType::TENSOR_QUANT8_SYMM:
                isSupported = false;
                break;
            default:
                break;
        }
        return isSupported;
    };

    auto isConstantTensor = [](auto& operand) -> bool {
        if (operand.lifetime == OperandLifeTime::CONSTANT_COPY ||
            operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE)
            return true;
        else
            return false;
    };

    auto getOpeandPtr = [&model](auto& operand) -> auto {
        auto& location = operand.location;
        return model.operandValues.data() + location.offset;
    };

    bool isSupport = true;
    // each operation check
    switch (operation.type) {
        // TODO: remove all of the work around
        case OperationType::CONV_2D: {
            auto& input = model.operands[operation.inputs[0]];
            auto& weight = model.operands[operation.inputs[1]];
            auto& bias = model.operands[operation.inputs[2]];
            if (isConstantTensor(input)) {
                LOG(INFO) << "Device don't support constant input";
                isSupport &= false;
            }

            isSupport &= (!isConstantTensor(bias) && !isConstantTensor(weight)) ||
                         (isConstantTensor(bias) && isConstantTensor(weight));
            break;
        }
        case OperationType::AVERAGE_POOL_2D:
        case OperationType::MAX_POOL_2D:
        case OperationType::SOFTMAX: {
            auto& input = model.operands[operation.inputs[0]];
            if (isConstantTensor(input)) {
                LOG(INFO) << "Device don't support constant input";
                isSupport &= false;
            }
            break;
        }
        case OperationType::LSH_PROJECTION: {
            auto typePtr = getOpeandPtr(model.operands[operation.inputs[3]]);
            if (3 == *(int32_t*)typePtr) isSupport &= false;
            break;
        }
        case OperationType::TANH: {
            if (OperandType::TENSOR_FLOAT32 != model.operands[operation.inputs[0]].type)
                isSupport &= false;
            break;
        }
        case OperationType::LSTM: {
            if (operation.inputs.size() > 23) isSupport &= false;
            break;
        }
        case OperationType::RESIZE_BILINEAR: {
            auto& scalarOperand = model.operands[operation.inputs[1]];
            if (OperandType::INT32 != scalarOperand.type) isSupport &= false;
            break;
        }
        case OperationType::TRANSPOSE: {
            // according to the spec, perm is optinal.
            if (operation.inputs.size() == 1) isSupport &= false;

            auto& perm = model.operands[operation.inputs[1]];
            if (OperandLifeTime::MODEL_INPUT == perm.lifetime) {
                LOG(ERROR) << "do not support perm as input";
                isSupport &= false;
            }
            size_t dimSize = perm.location.length / sizeof((int32_t)0);
            if (dimSize < 4) {
                isSupport &= true;
                break;
            }

            struct VsiRTInfo rt;
            auto permData = getOperandDataPtr(model, perm, rt);
            isSupport &= permData && (*(int32_t*)permData == 0);
            break;
        }
        case OperationType::FULLY_CONNECTED: {
            auto& input = model.operands[operation.inputs[0]];
            auto& weight = model.operands[operation.inputs[1]];
            if (input.dimensions.size() != 2 ||
                (weight.dimensions.size() == 2 && input.dimensions[1] != weight.dimensions[1]))
                isSupport &= false;
            break;
        }
        case OperationType::PAD: {
            // TODO: support pad at channel and batch
            auto& pad = model.operands[operation.inputs[1]];
            size_t dimSize = pad.location.length / sizeof((int32_t)0) / 2;
            if (dimSize < 3) {
                isSupport &= true;
                break;
            }

            struct VsiRTInfo rt;
            auto padData = reinterpret_cast<const int32_t*>(getOperandDataPtr(model, pad, rt));

            if (!padData) isSupport &= false;
            if (dimSize > 2) {
                if (dimSize == 3 && padData[4] + padData[5] != 0) isSupport &= false;
                if (dimSize == 4 && padData[6] + padData[7] + padData[0] + padData[1] != 0)
                    isSupport &= false;
            }
            break;
        }
        // to-do: check operand with operation
        // API 29 newly added operataion
        case OperationType::ABS:
        case OperationType::ARGMAX:
        case OperationType::ARGMIN:
        case OperationType::AXIS_ALIGNED_BBOX_TRANSFORM:
        case OperationType::BIDIRECTIONAL_SEQUENCE_LSTM:
        case OperationType::BIDIRECTIONAL_SEQUENCE_RNN:
        case OperationType::BOX_WITH_NMS_LIMIT:
        case OperationType::CAST:
        case OperationType::CHANNEL_SHUFFLE:
        case OperationType::DETECTION_POSTPROCESSING:
        case OperationType::EQUAL:
        case OperationType::EXP:
        case OperationType::EXPAND_DIMS:
        case OperationType::GATHER:
        case OperationType::GENERATE_PROPOSALS:
        case OperationType::GREATER:
        case OperationType::GREATER_EQUAL:
        case OperationType::GROUPED_CONV_2D:
        case OperationType::HEATMAP_MAX_KEYPOINT:
        case OperationType::INSTANCE_NORMALIZATION:
        case OperationType::LESS:
        case OperationType::LESS_EQUAL:
        case OperationType::LOGICAL_AND:
        case OperationType::LOGICAL_NOT:
        case OperationType::LOGICAL_OR:
        case OperationType::LOG_SOFTMAX:
        case OperationType::LOG:
        case OperationType::MAXIMUM:
        case OperationType::MINIMUM:
        case OperationType::NEG:
        case OperationType::NOT_EQUAL:
        case OperationType::PAD_V2:
        case OperationType::POW:
        case OperationType::PRELU:
        case OperationType::QUANTIZE:
        case OperationType::QUANTIZED_16BIT_LSTM:
        case OperationType::RANDOM_MULTINOMIAL:
        case OperationType::REDUCE_ALL:
        case OperationType::REDUCE_ANY:
        case OperationType::REDUCE_MAX:
        case OperationType::REDUCE_MIN:
        case OperationType::REDUCE_PROD:
        case OperationType::REDUCE_SUM:
        case OperationType::ROI_ALIGN:
        case OperationType::ROI_POOLING:
        case OperationType::RSQRT:
        case OperationType::SELECT:
        case OperationType::SIN:
        case OperationType::SLICE:
        case OperationType::SPLIT:
        case OperationType::SQRT:
        case OperationType::TILE:
        case OperationType::TOPK_V2:
        case OperationType::TRANSPOSE_CONV_2D:
        case OperationType::UNIDIRECTIONAL_SEQUENCE_LSTM:
        case OperationType::UNIDIRECTIONAL_SEQUENCE_RNN:
        case OperationType::RESIZE_NEAREST_NEIGHBOR:
            isSupport &= false;
            break;
        default:
            isSupport &= true;
    }

    // Overall check
    std::vector<OperationType> whiteList = {OperationType::ADD,
                                            OperationType::SUB,
                                            OperationType::MUL,
                                            OperationType::DIV,
                                            OperationType::MAXIMUM,
                                            OperationType::MINIMUM,
                                            OperationType::CONCATENATION,
                                            OperationType::CONV_2D,
                                            OperationType::FULLY_CONNECTED,
                                            OperationType::LSTM,
                                            OperationType::DEPTHWISE_CONV_2D};

    // do not support constant tensor as operation's Input except whiteList.
    if (std::find(whiteList.begin(), whiteList.end(), operation.type) == whiteList.end()) {
        if (isConstantTensor(model.operands[operation.inputs[0]])) isSupport &= false;
    }

    // TODO: [NNRT-1] Support static shape inference for NNAPI 1.2
    for (size_t i = 0; i < operation.outputs.size(); i++) {
        auto& dims = model.operands[operation.outputs[i]].dimensions;
        for (auto dimIndex : dims)
            if (dimIndex == 0) isSupport &= false;
    }

    // TODO: nnapi 1.2 new operand type
    for (size_t i = 0; i < operation.inputs.size(); i++) {
        auto& operand = model.operands[operation.inputs[i]];
        if (false == checkSupportedOperand(operand)) isSupport &= false;
    }
    for (size_t i = 0; i < operation.outputs.size(); i++) {
        auto& operand = model.operands[operation.outputs[i]];
        if (false == checkSupportedOperand(operand)) isSupport &= false;
    }
    return isSupport;
#endif
    return true;
}

#if ANDROID_SDK_VERSION > 28
Return<void> VsiDriver::getCapabilities_1_2(V1_2::IDevice::getCapabilities_1_2_cb _hidl_cb) {
    static const PerformanceInfo kPerf = {.execTime = 0.9f, .powerUsage = 0.9f};
    V1_2::Capabilities capabilities;
    capabilities.relaxedFloat32toFloat16PerformanceScalar = kPerf;
    capabilities.relaxedFloat32toFloat16PerformanceTensor = kPerf;
    // Set the base value for all operand types
    capabilities.operandPerformance = nonExtensionOperandPerformance({FLT_MAX, FLT_MAX});

    // Load supported operand types
    update(&capabilities.operandPerformance, OperandType::TENSOR_QUANT8_ASYMM, kPerf);
    if (!disable_float_feature_) {
        update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT32, kPerf);
        update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT16, kPerf);
    }
    _hidl_cb(ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> VsiDriver::getSupportedOperations_1_2(
    const V1_2::Model& model, V1_2::IDevice::getSupportedOperations_1_2_cb _hidl_cb) {
    if (!validateModel(model)) {
        LOG(ERROR) << __FUNCTION__;
        std::vector<bool> supported;
        _hidl_cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }
    return getSupportedOperationsBase(model, _hidl_cb);
}
#endif
}  // namespace vsi_driver
}  // namespace nn
}

using android::nn::vsi_driver::VsiDriver;
using android::sp;

int main() {
    sp<VsiDriver> driver(new VsiDriver());
    return driver->run();
}
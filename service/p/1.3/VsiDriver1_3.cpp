#include "VsiDriver.h"
#include "VsiPlatform.h"

#include "CapabilityConfig.h"

namespace android {
namespace nn {
namespace vsi_driver {

V1_2::OperandType VsiDriver::convertOperandTypeToV1_2(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
            return V1_2::OperandType::FLOAT32;
        case OperandType::INT32:
            return V1_2::OperandType::INT32;
        case OperandType::UINT32:
            return V1_2::OperandType::UINT32;
        case OperandType::TENSOR_FLOAT32:
            return V1_2::OperandType::TENSOR_FLOAT32;
        case OperandType::TENSOR_INT32:
            return V1_2::OperandType::TENSOR_INT32;
        case OperandType::TENSOR_QUANT8_ASYMM:
            return V1_2::OperandType::TENSOR_QUANT8_ASYMM;
        case OperandType::TENSOR_QUANT16_SYMM:
            return V1_2::OperandType::TENSOR_QUANT16_SYMM;
        case OperandType::TENSOR_FLOAT16:
            return V1_2::OperandType::TENSOR_FLOAT16;
        case OperandType::TENSOR_BOOL8:
            return V1_2::OperandType::TENSOR_BOOL8;
        case OperandType::FLOAT16:
            return V1_2::OperandType::FLOAT16;
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
            return V1_2::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL;
        case OperandType::TENSOR_QUANT16_ASYMM:
            return V1_2::OperandType::TENSOR_QUANT16_ASYMM;
        case OperandType::TENSOR_QUANT8_SYMM:
            return V1_2::OperandType::TENSOR_QUANT8_SYMM;
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case OperandType::SUBGRAPH: {
            LOG(ERROR) << __FUNCTION__ << "NNAPI 1.2 not support the type: " << toString(type);
            return V1_2::OperandType::OEM;
        }
        default: {
            LOG(ERROR) << __FUNCTION__ << "NNAPI 1.3 not support the type: " << toString(type);
            return V1_2::OperandType::OEM;
        }
    }
}

Return<void> VsiDriver::getCapabilities_1_3(V1_3::IDevice::getCapabilities_1_3_cb _hidl_cb) {
    static const PerformanceInfo kPerf = {.execTime = 0.9f, .powerUsage = 0.9f};
    V1_3::Capabilities capabilities;
    capabilities.relaxedFloat32toFloat16PerformanceScalar = kPerf;
    capabilities.relaxedFloat32toFloat16PerformanceTensor = kPerf;

    static const PerformanceInfo kIfWhilePerf = {.execTime = FLT_MAX, .powerUsage = FLT_MAX};
    capabilities.ifPerformance = kIfWhilePerf;
    capabilities.whilePerformance = kIfWhilePerf;
    // Set the base value for all operand types
    capabilities.operandPerformance =
        nonExtensionOperandPerformance<HalVersion::V1_3>({FLT_MAX, FLT_MAX});

    // Load supported operand types
    update(&capabilities.operandPerformance, OperandType::TENSOR_QUANT8_ASYMM, kPerf);
    update(&capabilities.operandPerformance, OperandType::TENSOR_BOOL8, kPerf);
    update(&capabilities.operandPerformance, OperandType::TENSOR_INT32, kPerf);

    if (!disable_float_feature_) {
        update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT32, kPerf);
        update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT16, kPerf);
    }

    auto customer_caps = CustomizeOverlay();
    for (auto& override_cap : customer_caps) {
        LOG(INFO) << "update capability for datatype(" << toString(override_cap.first)
                  << ")=" << override_cap.second.execTime << ", " << override_cap.second.powerUsage;
        update(&capabilities.operandPerformance, override_cap.first, override_cap.second);
    }

    _hidl_cb(V1_3::ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> VsiDriver::getSupportedOperations_1_3(
    const V1_3::Model& model, V1_3::IDevice::getSupportedOperations_1_3_cb _hidl_cb) {
    if (!validateModel(model)) {
        LOG(ERROR) << __FUNCTION__;
        std::vector<bool> supported;
        _hidl_cb(V1_3::ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }

    LOG(INFO) << "getSupportedOperations_1_3";
    if (validateModel(model)) {
        const size_t count = model.main.operations.size();
        std::vector<bool> supported(count, true);
        std::string notSupportReason = "";
        for (size_t i = 0; i < count; i++) {
            const auto& operation = model.main.operations[i];
            supported[i] = findExtensionOperation(operation) ||
                           (!IsOpBlocked(static_cast<int32_t>(operation.type)) &&
                            isSupportedOperation(operation, model, notSupportReason));
        }
        LOG(INFO) << notSupportReason;
        _hidl_cb(V1_3::ErrorStatus::NONE, supported);
    } else {
        LOG(ERROR) << "invalid model";
        std::vector<bool> supported;
        _hidl_cb(V1_3::ErrorStatus::INVALID_ARGUMENT, supported);
    }
    LOG(INFO) << "getSupportedOperations_1_3 exit";
    return Void();
}
}
}
}

#include "VsiDriver.h"
#include "VsiPlatform.h"
#include "VsiLock.h"

#include "CapabilityConfig.h"

namespace android {
namespace nn {
namespace vsi_driver {

Return<void> VsiDriver::getSupportedExtensions(V1_2::IDevice::getSupportedExtensions_cb _hidl_cb) {
    _hidl_cb(ErrorStatus::NONE,
             {
                 {.name = VSI_NPU_LOCK_EXTENSION_NAME},
             });
    return Void();
}

Return<void> VsiDriver::getCapabilities_1_2(V1_2::IDevice::getCapabilities_1_2_cb _hidl_cb) {
    static const PerformanceInfo kPerf = {.execTime = 0.9f, .powerUsage = 0.9f};
    V1_2::Capabilities capabilities;
    capabilities.relaxedFloat32toFloat16PerformanceScalar = kPerf;
    capabilities.relaxedFloat32toFloat16PerformanceTensor = kPerf;
    // Set the base value for all operand types
#if ANDROID_NN_API >= 30
    capabilities.operandPerformance =
        nonExtensionOperandPerformance<HalVersion::V1_2>({FLT_MAX, FLT_MAX});
#else
    capabilities.operandPerformance = nonExtensionOperandPerformance({FLT_MAX, FLT_MAX});
#endif

    // Load supported operand types
    update(&capabilities.operandPerformance, V1_2::OperandType::TENSOR_QUANT8_ASYMM, kPerf);
    update(&capabilities.operandPerformance, V1_2::OperandType::TENSOR_BOOL8, kPerf);
    update(&capabilities.operandPerformance, V1_2::OperandType::TENSOR_INT32, kPerf);

    if (!disable_float_feature_) {
        update(&capabilities.operandPerformance, V1_2::OperandType::TENSOR_FLOAT32, kPerf);
        update(&capabilities.operandPerformance, V1_2::OperandType::TENSOR_FLOAT16, kPerf);
    }

    auto customer_caps = CustomizeOverlay();
    for (auto& override_cap : customer_caps) {
#if ANDROID_NN_API >= 30
        auto operand_type = convertOperandTypeToV1_2(override_cap.first);
#else
        auto operand_type = override_cap.first;
#endif
        LOG(INFO) << "update capability for datatype(" << toString(operand_type)
                  << ")=" << override_cap.second.execTime << ", " << override_cap.second.powerUsage;
        update(&capabilities.operandPerformance, operand_type, override_cap.second);
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
    return getSupportedOperationsBase(HalPlatform::convertVersion(model), _hidl_cb);
    }
}
}
}

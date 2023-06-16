#include "VsiDriver_99.h"
#include "VsiPlatform.h"

namespace android {
namespace nn {
namespace vsi_driver {

    Return<void> VsiDriver::getCapabilities_1_1(V1_1::IDevice::getCapabilities_1_1_cb cb) {
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
        bool is_md5_matched = false;
        int model_size_block_level = getSystemPropertyAsInt("MODEL_BLOCK_LEVEL", 2);
        if (model_size_block_level < 0 || model_size_block_level > 7) {
            LOG(FATAL) << "MODEL_BLOCK_LEVEL should be any value of {0, 1, 2, 3, 4, 5, 6} \n"
                          "block_level = 6: reject model with size XXL,XL,L,M,S,XS\n"
                          "block_level = 5: reject model with size XXL,XL,L,M,S\n"
                          "block_level = 4: reject model with size XXL,XL,L,M\n"
                          "block_level = 3: reject model with size XXL,XL,L\n"
                          "block_level = 2: reject model with size XXL,XL\n"
                          "block_level = 1: reject model with size XXL\n"
                          "block_level = 0: don't reject model";
        }
        if (!validateModel(model)) {
            LOG(ERROR) << __FUNCTION__;
            std::vector<bool> supported;
            cb(ErrorStatus::INVALID_ARGUMENT, supported);
            return Void();
        }
    if (validateModel(model)) {
        const size_t count = model.operations.size();
        std::vector<bool> supported(count, true);
        std::string notSupportReason = "";
        for (size_t i = 0; i < count; i++) {
            const auto& operation = model.operations[i];
            if (weight_md5_check || model_size_block_level) {
                is_md5_matched = isWeightMd5Matched(operation, model, model_size_block_level);
                if (is_md5_matched) break;
            }
            supported[i] = (!IsOpBlocked(static_cast<int32_t>(operation.type)) &&
                            isSupportedOperation(operation, model, notSupportReason));
        }
        if (is_md5_matched) {
            LOG(INFO) << "Weight MD5 matched, reject the whole model.";
            for (size_t i = 0; i < count; ++i) {
                supported[i] = false;
            }
        }
        LOG(INFO) << notSupportReason;
        cb(ErrorStatus::NONE, supported);
    } else {
        LOG(ERROR) << "invalid model";
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
    }
        return Void();
    }
}
}
}

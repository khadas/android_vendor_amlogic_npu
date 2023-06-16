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
#include "WeightMd5CheckingConfig.h"


namespace android {
namespace nn {
namespace vsi_driver {

namespace model_size {
static const std::vector<std::string> D = {
    "BF619EAC0CDF3F68D496EA9344137E8B",
    "AAAD27F00E113AB3BA1D63DAFC700CB0",
    "C1A00C83A14B04438A92A6ECE4F50F70",
    // unkown
    // "9210F76EF8EB7F68AE6C1F981CA41B1B",
    // "B2FC969768D6A8455C157BA01FC80FBC",
    // "EF8559A3BC4FA97E4CC66949BA6F5094",
    // "70D1D444832DDAF9F4EA473E5EEF283B",
    // "63EE8D994B664CD904379E349A4A041D",
    // "33ED56ED2C1E6F9A49F6C71FAE95158C",
    // "B6A582A234A1A7DE6F39403DD199683D",
    // "AAAD27F00E113AB3BA1D63DAFC700CB0",
    // "C92A719325D0D2C1777EDD8341F25D80",
    // "C1A00C83A14B04438A92A6ECE4F50F70",
    // "7D596461C457E97BF36E65EC5CCD271E",
    // "BF619EAC0CDF3F68D496EA9344137E8B",
};
static const std::vector<std::string> XXL = {
    // pynet_float
    // srgan_float
    "982548CB17D7C2E844A883ED342BA28E",
    "DFF7E53C34B86F3D8C10EE35E9C872A2",
    "4187083B2E58D0BDA414C128057D7333",
    "60C0780C3D8E60FD94829E45F3ED9C25",
    "33ED56ED2C1E6F9A49F6C71FAE95158C",
    "AAAD27F00E113AB3BA1D63DAFC700CB0",
    "C1A00C83A14B04438A92A6ECE4F50F70",
    "30BE5212884AC51654CC27EB735E1074",
    "7C387225B0395150054B2606467D264C",
    // vgg_float
    "6D8E7DB833A9D85B9803C8FD61368CCD",
    "B1BFA06C99E929CCAF7835A4CFA10E87",
    "5D311502BF247500C64CE942BA8B8071",
    "14EE8F7B5C4492159470CBD66CFE545A",
    // dped_float
    "DFF7E53C34B86F3D8C10EE35E9C872A2",
    // unet_float
    "5D311502BF247500C64CE942BA8B8071",
    // dped_float
};
static const std::vector<std::string> XL = {
    // inception_v3_float
    "52EA8633678B9D9A05485259D834F455",
    // inception_face_float
    "3C1812918369446168A511D2E2207599",
    // inception_face_quant
    "3071BB800BEE3652D7F38BEF08EE40DE",
    // deeplab_v3_plus_float
    "2E454195AE67090515C4DA9C02D58FA2",
};
static const std::vector<std::string> L = {
    // crnn_float
};
static const std::vector<std::string> M = {
    // mobilenet_v3_float
    // srcnn_float
};
static const std::vector<std::string> S = {
    // mobilenet_v2_float
    "7A6457A8F3BB2A1EEA9AFAA9110733D2",
    // lstm_float
    // mobilenet_v1_0.25_128: 1001x1x1x256
    "D1A49E737E8C59700DC98FB4B0056BD0",
    // mobilenet_v1_0.5_160
    "F40F00A93B0BFF0971430644493641F8",
    // mobilenet_v1_0.75_192
    "07A1A75E2F3FDB7415367B2B6A5D44D6",
    // mobilenet_v1_1.0_224
    "6A3834EE93D674E7A016C19464FA5514",
    // mobilenet_v2_0.75_192
    "DAAC1E40DD63B67003671E8E63B7338F",
};
static const std::vector<std::string> XS = {
    // mobilenet_v1_0.75_192_quant
    "74A952AF6354845236115920BD62D711",
    // mobilenet_v1_0.5_160_quant
    "C6406F6AB18BD32E2A8C07A8FB366C40",
     // mobilenet_v1_0.25_128_quant: 1001x1x1x256
    "4310747F8368C75264E89AD2B75699C2",
    // ssd_mobilenet_v2_coco_quant
    "B4146FC276859FAFFCE68FF52B1FAF66",
    // srgan_quant
    "7242EE970D48CA8AE2638579EF31CC4F",
    // vgg_quant
    "98355ABE3A91B0984C414D71B9A707CC",
    // pynet_quant
    "B9CD20A2DE17D5781F4E396385DF0326",
    // dped_quant
    "0C5D588678EEE63B96132A004473B1D1",
    // inception_v4_299_quant
    "8B4DEA7E17A4B928D6D5B4BAD7D89F12",
    // tf_arcface_100_v1_quant
    "E512A03678E81DD73E20EE26CDEECDE6",
    // unet_quant
    "A9700F507EF8B50D90FDFA3973333DD4",
    // inception_v3_quant
    "97934CD9778FEA2B0E015485129F0E25",
    // inception_v2_224_quant
    "A5EA28AE924AE28974708D3321439DB6",
    // mobilenet_v2_quant
    "1C4903CAD7FDD5827B6E01FD588237AA",
    // deeplab_v3_plus_quant
    "05441AB9B63D637A4BE1DE5EED00B505"
    // inception_v1_224_quant
    "22B06D17F4DF7D71CCA0C6A421232006",
    // mobilenet_v2_b4_quant
    "1C4903CAD7FDD5827B6E01FD588237AA",
     // mobilenet_v3_quant
    "E7864733CDAEDF35A0FA6E442264DC0D",
    // mobilenet_v1_1.0_224_quant
    "AAD0A219FB32747A68F6FEAC20126B47",
};
}  // namespace model_size

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
        const uint8_t kLowBitsType = static_cast<uint8_t>(V1_2::Model::ExtensionTypeEncoding::LOW_BITS_TYPE);
        const uint32_t kTypeWithinExtensionMask = (1 << kLowBitsType) - 1;
        uint16_t extensionSupportMask = static_cast<int32_t>(operationType) >> kLowBitsType;
        uint16_t typeWithinExtension = static_cast<int32_t>(operationType) & kTypeWithinExtensionMask;
        if (extensionSupportMask != 0) return typeWithinExtension;
        return 0;
    };
#endif

#if ANDROID_SDK_VERSION >= 30
    V1_2::OperandType convertOperandTypeToV1_2(OperandType type);
    Return<void> getCapabilities_1_3(getCapabilities_1_3_cb _hidl_cb);
    Return<void> getSupportedOperations_1_3(const V1_3::Model& model,getSupportedOperations_1_3_cb _hidl_cb);
#endif
    static bool isSupportedOperation(const HalPlatform::Operation& operation,
                                     const HalPlatform::Model& model,
                                     std::string& not_support_reason);

    static const uint8_t* getOperandDataPtr( const HalPlatform::Model& model,
                                             const HalPlatform::Operand& hal_operand,
                                             VsiRTInfo &vsiMemory);

    bool isWeightMd5Matched(const HalPlatform::Operation& operation,
                            const HalPlatform::Model& model,
                            int block_level);

   private:
   int32_t disable_float_feature_; // switch that float-type running on hal
   private:
    void initalizeEnv();

    template <typename T_model, typename T_getSupportOperationsCallback>
    Return<void> getSupportedOperationsBase(const T_model& model,
                                            T_getSupportOperationsCallback cb){
        LOG(INFO) << "getSupportedOperations";
        bool is_md5_matched = false;
        int model_size_block_level = getSystemPropertyAsInt("MODEL_BLOCK_LEVEL", 1);
        if (model_size_block_level < 0 || model_size_block_level > 4) {
            LOG(FATAL) << "MODEL_BLOCK_LEVEL should be any value of {0, 1, 2, 3} \n"
                          "block_level = 3: reject model with size L,M,S\n"
                          "block_level = 2: reject model with size L,M\n"
                          "block_level = 1: reject model with size L\n"
                          "block_level = 0: don't reject model";
        }

        if (validateModel(model)) {
#if ANDROID_SDK_VERSION < 30
            const size_t count = model.operations.size();
#elif ANDROID_SDK_VERSION >=30
            const size_t count = model.main.operations.size();
#endif
            std::vector<bool> supported(count, true);
            std::string notSupportReason = "";
            for (size_t i = 0; i < count; i++) {
#if ANDROID_SDK_VERSION < 30
                const auto& operation = model.operations[i];
#elif ANDROID_SDK_VERSION >= 30
                const auto& operation = model.main.operations[i];
#endif
                if (weight_md5_check || model_size_block_level) {
                    if ((is_md5_matched =
                             isWeightMd5Matched(operation, model, model_size_block_level)))
                        break;
                }
#if ANDROID_SDK_VERSION >= 29
                supported[i] = findExtensionOperation(operation) ||
                               (!IsOpBlocked(static_cast<int32_t>(operation.type)) &&
                                isSupportedOperation(operation, model, notSupportReason));
#else
                supported[i] = !IsOpBlocked(static_cast<int32_t>(operation.type)) &&
                               isSupportedOperation(operation, model, notSupportReason);
#endif
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
        LOG(INFO) << "getSupportedOperations exit";
        return Void();
    };

};

}
}
}
#endif

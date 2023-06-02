#ifndef ANDROID_ML_NN_VSI_DRIVER_H
#error "Don't include this file in current file, its not intended"
#endif

// Chip vendor decided capability for each data type

namespace android {
namespace nn {
namespace vsi_driver {

static std::vector<std::pair<OperandType, PerformanceInfo>> CustomizeOverlay() {
    std::vector<std::pair<OperandType, PerformanceInfo>> caps;
    // caps.push_back(std::make_pair(OperandType::TENSOR_FLOAT32, PerformanceInfo{FLT_MAX, FLT_MAX}));
    return caps;
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

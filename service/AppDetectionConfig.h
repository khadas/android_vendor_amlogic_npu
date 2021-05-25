#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>

// For Android
#include <sys/system_properties.h>

#ifndef ANDROID_ML_NN_VSI_DRIVER_H
#error "Don't include this file in current file, its not intended"
#endif

// Chip vendor decided capability for each data type

namespace android {
namespace nn {
namespace vsi_driver {
bool GetProcessName(std::vector<std::string>& process_name) {
    std::string cmd = "ps -a | awk '{print $9}'";
    FILE *fd = popen(cmd.c_str(), "r");
    if (!fd) {
        LOG(ERROR) << __FUNCTION__ << "Execute shell cmd failed.";
        return false;
    }
    char tmp[1024];
    while (fgets(tmp, sizeof(tmp), fd) != nullptr) {
        if (tmp[strlen(tmp) - 1] == '\n') {
            tmp[strlen(tmp) - 1] = '\0';
        }
        process_name.push_back(tmp);
    }
    pclose(fd);
    return true;
}

bool AppDetected(std::string app) {
    std::vector<std::string> process_name;
    if (GetProcessName(process_name)) {
        for (auto& name : process_name) {
            if (strstr(name.c_str(), app.c_str())) {
                return true;
            }
        }
    } else {
        LOG(ERROR) << __FUNCTION__ << "Get process name failed.";
        return false;
    }
    return false;
}

std::vector<std::pair<OperandType, PerformanceInfo>> AppDetectionOverlay() {
    std::vector<std::pair<OperandType, PerformanceInfo>> caps;
    // if (AppDetected("CtsNNAPITestCases")) {
    //     caps.push_back(std::make_pair(OperandType::TENSOR_FLOAT32, PerformanceInfo{FLT_MAX, FLT_MAX}));
    //     LOG(INFO) << __FUNCTION__ << ":Float32 has been disabled.";
    // }
    return caps;
}
}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

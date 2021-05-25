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

#include "VsiBurstExecutor.h"
#include "VsiDriver.h"
#include "VsiPreparedModel.h"
#include "VsiLock.h"

#include <sys/mman.h>

#include "OperationsUtils.h"
#include "ValidateHal.h"

#include "nnrt/event.hpp"
#include "nnrt/error.hpp"


namespace android {
namespace nn {
namespace vsi_driver {

using time_point = std::chrono::steady_clock::time_point;
static const Timing kNoTiming = {.timeOnDevice = UINT64_MAX, .timeInDriver = UINT64_MAX};
std::atomic_bool isLockDevices = 0;

auto now() {
    return std::chrono::steady_clock::now();
};

auto microsecondsDuration(decltype(now()) end, decltype(now()) start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
};

static Return<ErrorStatus> convertResultCodeToErrorStatus(int resultCode) {
    switch (resultCode) {
        case NNA_NO_ERROR:
            return ErrorStatus::NONE;

        case NNA_BAD_DATA:
        case NNA_UNEXPECTED_NULL:
            LOG(ERROR) << "INVALID_ARGUMENT";
            return ErrorStatus::INVALID_ARGUMENT;

        case NNA_OUTPUT_INSUFFICIENT_SIZE:
            LOG(ERROR) << "output insufficient size";
            return ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;

        default:
            LOG(ERROR) << "Unknown result code " << resultCode
                       << " mapped to ErrorStatus::GENERAL_FAILURE";
            return ErrorStatus::GENERAL_FAILURE;
        case NNA_BAD_STATE:
        case NNA_INCOMPLETE:
        case NNA_OP_FAILED:
        case NNA_OUT_OF_MEMORY:
        case NNA_UNMAPPABLE:
            LOG(ERROR) << "GENERAL_FAILURE";
            return ErrorStatus::GENERAL_FAILURE;
    }
}

Return<void> VsiPreparedModel::notify(const sp<V1_0::IExecutionCallback>& callback,
                                      const ErrorStatus& status,
                                      const hidl_vec<OutputShape>&,
                                      Timing) {
    return callback->notify(status);
}

Return<void> VsiPreparedModel::notify(const sp<V1_2::IExecutionCallback>& callback,
                                      const ErrorStatus& status,
                                      const hidl_vec<OutputShape>& outputShapes,
                                      Timing timing) {
    return callback->notify_1_2(status, outputShapes, timing);
}

void VsiPreparedModel::notify(const V1_2::IPreparedModel::executeSynchronously_cb& callback,
                              const ErrorStatus& status,
                              const hidl_vec<OutputShape>& outputShapes,
                              Timing timing) {
    callback(status, outputShapes, timing);
}

Return<void> VsiPreparedModel::configureExecutionBurst(
    const sp<V1_2::IBurstCallback>& callback,
    const android::hardware::MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
    const android::hardware::MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
    configureExecutionBurst_cb cb) {
    // just cache the hidl model, and prepare it in BurstExecutorWithCache
    // TODO: cache the meachine code
    std::shared_ptr<BurstExecutorWithCache> executorWithCache =
        std::make_shared<BurstExecutorWithCache>(model_, preference_);
    const sp<V1_2::IBurstContext> burst =
        ExecutionBurstServer::create(callback, requestChannel, resultChannel, executorWithCache);

    if (burst == nullptr) {
        cb(ErrorStatus::GENERAL_FAILURE, {});
    } else {
        cb(ErrorStatus::NONE, burst);
    }

    return Void();
};

Return<ErrorStatus> VsiPreparedModel::executeBase(const Request& request,
                                                  MeasureTiming measure,
                                                  ErrorStatus& oStatus,
                                                  hidl_vec<OutputShape>& oOutputShapes,
                                                  Timing& oTiming) {
#if ANDROID_SDK_VERSION >= 29
    for (const auto& hal_op : model_.operations) {
        if (VsiDriver::findExtensionOperation(hal_op) == VSI_NPU_UNLOCK) {
            isLockDevices = false;
            return ErrorStatus::NONE;
        }
    }
    if (isLockDevices) return ErrorStatus::DEVICE_UNAVAILABLE;
#endif

    Timing timing = kNoTiming;
    time_point deviceStart;
    deviceStart = now();

    if (!validateRequest(request, model_)) {
        LOG(ERROR) << "invalid request";
        oStatus = ErrorStatus::INVALID_ARGUMENT;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = kNoTiming;
        return ErrorStatus::INVALID_ARGUMENT;
    }

    if (nullptr == native_compile_) {
        native_compile_.reset(new nnrt::Compilation(native_model_.get()),
                              NativeDeleter<nnrt::Compilation>());
    }
    map_rtinfo_from_hidl_memory(request.pools, io_buffer_);

    native_exec_.reset(new nnrt::Execution(native_compile_.get()),
                       NativeDeleter<nnrt::Execution>());

    std::vector<hidl_memory> io_pools = request.pools;
    std::vector<RequestArgument> input_args = request.inputs;
    std::vector<RequestArgument> output_args = request.outputs;
    std::vector<OutputShape> outputShapes(request.outputs.size());

    update_operand_from_request(model_.inputIndexes, input_args);
    update_operand_from_request(model_.outputIndexes, output_args);
    if (!native_model_->isCompiled()) {
        native_compile_->run();
    }

    auto status = update_pool_info_from_request(
        request, model_.inputIndexes, input_args, IO::INPUT, outputShapes);
    if (ErrorStatus::NONE != status) {
        oStatus = status;
        oOutputShapes = outputShapes;
        oTiming = kNoTiming;
        return status;
    }

    status = update_pool_info_from_request(
        request, model_.outputIndexes, output_args, IO::OUTPUT, outputShapes);
    if (ErrorStatus::NONE != status) {
        oStatus = status;
        oOutputShapes = outputShapes;
        oTiming = timing;
        return status;
    }

    LOG(INFO) << "start compute";
    int error = native_exec_->compute();

    LOG(INFO) << "Finished compute";
    status = convertResultCodeToErrorStatus(error);
    release_rtinfo(io_buffer_);

    if (status != ErrorStatus::NONE) {
        oStatus = status;
        oOutputShapes = outputShapes;
        oTiming = kNoTiming;
        LOG(INFO) << "Error happened";
        return status;
    }

    time_point deviceEnd;
    deviceEnd = now();
    timing.timeOnDevice = uint64_t(microsecondsDuration(deviceEnd, deviceStart));
    timing.timeInDriver = uint64_t(microsecondsDuration(deviceEnd, deviceStart));

    LOG(INFO) << __FUNCTION__ << " time: " << toString(timing) << "micro-sec";

    oStatus = status;
    oOutputShapes = outputShapes;
    oTiming = MeasureTiming::YES == measure ? timing : kNoTiming;

    return ErrorStatus::NONE;
}

Return<void> VsiPreparedModel::executeSynchronously(const Request& request,
                                                    MeasureTiming measure,
                                                    executeSynchronously_cb cb) {
    ErrorStatus status;
    hidl_vec<OutputShape> outputShapes;
    Timing timing;

#if ANDROID_SDK_VERSION >= 29
    for (const auto& hal_op : model_.operations) {
        uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);
        if (extensionType == VSI_NPU_LOCK) {
            isLockDevices = true;
        } else if (extensionType == VSI_NPU_UNLOCK) {
            isLockDevices = false;
            cb(ErrorStatus::NONE, outputShapes, timing);
            native_exec_.reset();
            return Void();
        }
    }
    if (isLockDevices) {
        cb(ErrorStatus::DEVICE_UNAVAILABLE, outputShapes, timing);
        native_exec_.reset();
        return Void();
    }
#endif

    VsiPreparedModel::sandbox_.wait(
        [this, request, measure, &status, &outputShapes, &timing]() -> bool {
            this->executeBase(request, measure, status, outputShapes, timing);
            return false;
        });

    cb(status, outputShapes, timing);

    native_exec_.reset();
    return Void();
};

Return<ErrorStatus> VsiPreparedModel::execute_1_2(const Request& request,
                                                  MeasureTiming measure,
                                                  const sp<V1_2::IExecutionCallback>& callback) {
#if ANDROID_SDK_VERSION >= 29
    for (const auto& hal_op : model_.operations) {
        uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);
        if (extensionType == VSI_NPU_LOCK) {
            isLockDevices = true;
            return ErrorStatus::DEVICE_UNAVAILABLE;
        } else if (extensionType == VSI_NPU_UNLOCK) {
            isLockDevices = false;
        }
    }
    if (isLockDevices) {
        return ErrorStatus::DEVICE_UNAVAILABLE;
    }
#endif

    ErrorStatus status;
    hidl_vec<OutputShape> outputShapes;
    Timing timing;

    VsiPreparedModel::sandbox_.wait(
        [this, request, measure, &status, &outputShapes, &timing]() -> bool {
            this->executeBase(request, measure, status, outputShapes, timing);
            return false;
        });

    callback->notify_1_2(status, outputShapes, timing);
    native_exec_.reset();

    return status;
};

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

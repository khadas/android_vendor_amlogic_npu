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

#include <sys/mman.h>

#include <nnrt/error.hpp>
#include <nnrt/event.hpp>

#include "OperationsUtils.h"
#include "ValidateHal.h"
#include "VsiBurstExecutor.h"
#include "VsiDriver.h"
#include "VsiLock.h"
#include "VsiPreparedModel.h"

namespace android {
namespace nn {
namespace vsi_driver {

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
    // ONLY normal model can be here
    if (isDeviceLocked) return ErrorStatus::DEVICE_UNAVAILABLE;
#endif

    Timing timing = kNoTiming;
    time_point deviceStart;
    deviceStart = now();

#if ANDROID_SDK_VERSION >= 30
    if (!validateRequest(HalPlatform::convertVersion(request), model_)) {
#else
    if (!validateRequest(request, model_)) {
#endif
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

#if ANDROID_SDK_VERSION < 30
    update_operand_from_request(model_.inputIndexes, input_args);
    update_operand_from_request(model_.outputIndexes, output_args);
#elif ANDROID_SDK_VERSION >= 30
    update_operand_from_request(model_.main.inputIndexes, input_args);
    update_operand_from_request(model_.main.outputIndexes, output_args);
#endif
    if (!native_model_->isCompiled()) {
        native_compile_->run();
    }

#if ANDROID_SDK_VERSION < 30
    auto status = update_pool_info_from_request(
        request, model_.inputIndexes, input_args, IO::INPUT, outputShapes);
#elif ANDROID_SDK_VERSION >= 30
    auto status = update_pool_info_from_request(
        request, model_.main.inputIndexes, input_args, IO::INPUT, outputShapes);
#endif
    if (ErrorStatus::NONE != status) {
        oStatus = status;
        oOutputShapes = outputShapes;
        oTiming = kNoTiming;
        return status;
    }

#if ANDROID_SDK_VERSION < 30
    status = update_pool_info_from_request(
        request, model_.outputIndexes, output_args, IO::OUTPUT, outputShapes);
#elif ANDROID_SDK_VERSION >= 30
    status = update_pool_info_from_request(
        request, model_.main.outputIndexes, output_args, IO::OUTPUT, outputShapes);
#endif
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
    // idle -> lock -> lock - > idle
#if ANDROID_SDK_VERSION < 30
    for (const auto& hal_op : model_.operations) {
#elif ANDROID_SDK_VERSION >= 30
    for (const auto& hal_op : model_.main.operations) {
#endif
            uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);

            if (isDeviceLocked) {
                if (VSI_NPU_LOCK == extensionType) {
                    // lock - > lock
                    cb(ErrorStatus::DEVICE_UNAVAILABLE, outputShapes, timing);
                    native_exec_.reset();
                    return Void();
                } else if (VSI_NPU_UNLOCK == extensionType) {
                    // lock -> unlock
                    isDeviceLocked = false;
                    cb(ErrorStatus::NONE, outputShapes, timing);
                    native_exec_.reset();
                    return Void();
                }
            } else {  // device not locked
                if (VSI_NPU_LOCK == extensionType) {
                    // unlock - > lock
                    isDeviceLocked = true;
                    cb(ErrorStatus::NONE, outputShapes, timing);
                    native_exec_.reset();
                    return Void();
                } else if (VSI_NPU_UNLOCK == extensionType) {
                    cb(ErrorStatus::NONE, outputShapes, timing);
                    native_exec_.reset();
                    return Void();
                }
            }
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
#if ANDROID_SDK_VERSION < 30
    for (const auto& hal_op : model_.operations) {
#elif ANDROID_SDK_VERSION >= 30
    for (const auto& hal_op : model_.main.operations) {
#endif
        uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);

        if (isDeviceLocked) {
            if (extensionType == VSI_NPU_LOCK) {
                return ErrorStatus::DEVICE_UNAVAILABLE;
            } else if (extensionType == VSI_NPU_UNLOCK) {
                isDeviceLocked = false;
                return ErrorStatus::NONE;
            }
        } else {  // device not locked
            if (extensionType == VSI_NPU_LOCK) {
                isDeviceLocked = true;
                return ErrorStatus::NONE;
            } else if (extensionType == VSI_NPU_UNLOCK) {
                return ErrorStatus::NONE;
            }
        }
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
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

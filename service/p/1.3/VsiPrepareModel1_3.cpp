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

//#include "VsiBurstExecutor.h"
#include "VsiDriver.h"
#include "VsiPreparedModel.h"
#include "VsiDevice.h"

#include <sys/mman.h>

#include "OperationsUtils.h"
#include "ValidateHal.h"

#include <nnrt/error.hpp>
#include <nnrt/event.hpp>

namespace android {
namespace nn {
namespace vsi_driver {

extern std::shared_ptr<BufferTracker> kBufferTracker;

Return<V1_3::ErrorStatus> VsiPreparedModel::mapRtinfoFromRequest(
        const V1_3::Request& request, std::vector<VsiRTInfo>& rtInfos) {
    const auto& request_v1_0 = HalPlatform::convertToV1_0(request);
    const auto& pools = request.pools;
    const auto& pools_v1_0 = request_v1_0.pools;
    rtInfos.clear();
    rtInfos.resize(pools.size());
    for (uint32_t i = 0; i < pools.size(); i++) {
        switch (pools[i].getDiscriminator()) {
            case V1_3::Request::MemoryPool::hidl_discriminator::hidlMemory: {
                if (!mapHidlMem(pools_v1_0[i], rtInfos[i])) {
                    return V1_3::ErrorStatus::INVALID_ARGUMENT;
                }
                break;
            }
            case V1_3::Request::MemoryPool::hidl_discriminator::token: {
                auto buffer_wrapper = kBufferTracker->get(pools[i].token());
                if (buffer_wrapper == nullptr) {
                    LOG(ERROR) << "Get token buffer failed.";
                    return V1_3::ErrorStatus::INVALID_ARGUMENT;
                }
                auto status = buffer_wrapper->validateRequest(i, request, this);
                if (V1_3::ErrorStatus::NONE != status) {
                    LOG(ERROR) << "Validate request failed.";
                    return status;
                }
                auto rtPool = buffer_wrapper->createRunTimePoolInfo();
                rtInfos[i].shared_mem = nullptr;
                rtInfos[i].mem_type = "token";
                rtInfos[i].ptr = rtPool.getBuffer();
                rtInfos[i].buffer_size = rtPool.getSize();
                rtInfos[i].graphic_buffer = nullptr;
                break;
            }
            default:
                LOG(ERROR) << "Not support memory type.";
                return V1_3::ErrorStatus::INVALID_ARGUMENT;
        }
    }
    return V1_3::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> VsiPreparedModel::updateDeivceMem(const V1_3::Request& request,
                                              const hidl_vec<OutputShape>& outputShapes) {
    for (uint32_t i = 0; i < request.outputs.size(); i++) {
        const uint32_t pool_index = request.outputs[i].location.poolIndex;
        const auto pool = request.pools[pool_index];
        if (V1_3::Request::MemoryPool::hidl_discriminator::token == pool.getDiscriminator()) {
            auto buffer_wrapper = kBufferTracker->get(pool.token());
            if (!buffer_wrapper->updateDimensions(outputShapes[i].dimensions)) {
                return V1_3::ErrorStatus::GENERAL_FAILURE;
            }
            buffer_wrapper->setInitialized(true);
        }
    }
    return V1_3::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> VsiPreparedModel::executeBase_1_3(const V1_3::Request& request,
                                                            MeasureTiming measure,
                                                            V1_3::ErrorStatus& oStatus,
                                                            hidl_vec<OutputShape>& oOutputShapes,
                                                            Timing& oTiming) {

    if (isDeviceLocked) return V1_3::ErrorStatus::DEVICE_UNAVAILABLE;

    V1_3::ErrorStatus status_1_3 = V1_3::ErrorStatus::NONE;

    Timing timing = kNoTiming;
    time_point deviceStart;
    deviceStart = now();

    if (!validateRequest(HalPlatform::convertVersion(request), model_)) {
        LOG(ERROR) << "invalid request";
        oStatus = V1_3::ErrorStatus::INVALID_ARGUMENT;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = kNoTiming;
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    if (nullptr == native_compile_) {
        native_compile_.reset(new nnrt::Compilation(native_model_.get()),
                              NativeDeleter<nnrt::Compilation>());
    }
    auto request_v1_0 = HalPlatform::convertToV1_0(request);
    status_1_3 = mapRtinfoFromRequest(request, io_buffer_);
    if (V1_3::ErrorStatus::NONE != status_1_3) {
        oStatus = status_1_3;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = kNoTiming;
        LOG(INFO) << "Map rtinfo failed.";
        return status_1_3;
    }

    native_exec_.reset(new nnrt::Execution(native_compile_.get()),
                       NativeDeleter<nnrt::Execution>());

    std::vector<RequestArgument> input_args = request.inputs;
    std::vector<RequestArgument> output_args = request.outputs;
    std::vector<OutputShape> outputShapes(request.outputs.size());
    update_operand_from_request(model_.main.inputIndexes, input_args);
    update_operand_from_request(model_.main.outputIndexes, output_args);

    if (!native_model_->isCompiled()) {
        native_compile_->run();
    }
    auto status = update_pool_info_from_request(
        request_v1_0, model_.main.inputIndexes, input_args, IO::INPUT, outputShapes);
    status_1_3 = HalPlatform::convertVersion(status);
    if (V1_3::ErrorStatus::NONE != status_1_3) {
        oStatus = status_1_3;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = kNoTiming;
        return status_1_3;
    }
    status = update_pool_info_from_request(
        request_v1_0, model_.main.outputIndexes, output_args, IO::OUTPUT, outputShapes);

    status_1_3 = HalPlatform::convertVersion(status);
    if (V1_3::ErrorStatus::NONE != status_1_3) {
        oStatus = status_1_3;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = timing;
        return status_1_3;
    }

    LOG(INFO) << "start compute";
    int error = native_exec_->compute();
    auto c_status = convertResultCodeToErrorStatus(error);
    status_1_3 = HalPlatform::convertVersion(c_status);
    if (V1_3::ErrorStatus::NONE != status_1_3) {
        oStatus = status_1_3;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = kNoTiming;
        LOG(INFO) << "Native compute fialed.";
        return status_1_3;
    }

    status_1_3 = updateDeivceMem(request, outputShapes);
    if (V1_3::ErrorStatus::NONE != status_1_3) {
        oStatus = status_1_3;
        oOutputShapes = std::vector<OutputShape>(0);
        oTiming = kNoTiming;
        LOG(INFO) << "Update devices memory fialed.";
        return status_1_3;
    }
    release_rtinfo(io_buffer_);

    time_point deviceEnd;
    deviceEnd = now();
    timing.timeOnDevice = uint64_t(microsecondsDuration(deviceEnd, deviceStart));
    timing.timeInDriver = uint64_t(microsecondsDuration(deviceEnd, deviceStart));

    LOG(INFO) << __FUNCTION__ << " time: " << toString(timing) << "micro-sec";

    oStatus = status_1_3;
    oOutputShapes = outputShapes;
    oTiming = MeasureTiming::YES == measure ? timing : kNoTiming;

    return V1_3::ErrorStatus::NONE;
}

hal::Return<hal::V1_3::ErrorStatus> VsiPreparedModel::execute_1_3(
    const hal::V1_3::Request& request,
    hal::MeasureTiming measure,
    const hal::OptionalTimePoint& deadline,
    const hal::OptionalTimeoutDuration& loopTimeoutDuration,
    const sp<hal::V1_3::IExecutionCallback>& callback) {
    V1_3::ErrorStatus status;
    hidl_vec<OutputShape> outputShapes;
    Timing timing;

    for (const auto& hal_op : model_.main.operations) {

        uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);

        if (isDeviceLocked) {
            if (extensionType == VSI_NPU_LOCK) {
                return V1_3::ErrorStatus::DEVICE_UNAVAILABLE;
            } else if (extensionType == VSI_NPU_UNLOCK) {
                isDeviceLocked = false;
                return V1_3::ErrorStatus::NONE;
            }
        } else {  // device not locked
            if (extensionType == VSI_NPU_LOCK) {
                isDeviceLocked = true;
                return V1_3::ErrorStatus::NONE;
            } else if (extensionType == VSI_NPU_UNLOCK) {
                return V1_3::ErrorStatus::NONE;
            }
        }
    }

    VsiPreparedModel::sandbox_.wait(
        [this, request, measure, &status, &outputShapes, &timing]() -> bool {
            this->executeBase_1_3(request, measure, status, outputShapes, timing);
            return false;
        });

    callback->notify_1_3(status, outputShapes, timing);
    native_exec_.reset();

    return status;
};

Return<void> VsiPreparedModel::executeSynchronously_1_3(
    const hal::V1_3::Request& request,
    hal::MeasureTiming measure,
    const hal::OptionalTimePoint& deadline,
    const hal::OptionalTimeoutDuration& loopTimeoutDuration,
    executeSynchronously_1_3_cb cb) {
    V1_3::ErrorStatus status;
    Timing timing;
    hidl_vec<OutputShape> outputShapes;
    // idle -> lock -> lock - > idle
    for (const auto& hal_op : model_.main.operations) {
        uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);

        if (isDeviceLocked) {
            if (VSI_NPU_LOCK == extensionType) {
                // lock - > lock
                cb(V1_3::ErrorStatus::DEVICE_UNAVAILABLE, outputShapes, timing);
                native_exec_.reset();
                return Void();
            } else if (VSI_NPU_UNLOCK == extensionType) {
                // lock -> unlock
                isDeviceLocked = false;
                cb(V1_3::ErrorStatus::NONE, outputShapes, timing);
                native_exec_.reset();
                return Void();
            }
        } else {  // device not locked
            if (VSI_NPU_LOCK == extensionType) {
                // unlock - > lock
                isDeviceLocked = true;
                cb(V1_3::ErrorStatus::NONE, outputShapes, timing);
                native_exec_.reset();
                return Void();
            } else if (VSI_NPU_UNLOCK == extensionType) {
                cb(V1_3::ErrorStatus::NONE, outputShapes, timing);
                native_exec_.reset();
                return Void();
            }
        }
    }

    VsiPreparedModel::sandbox_.wait(
        [this, request, measure, &status, &outputShapes, &timing]() -> bool {
            this->executeBase_1_3(request, measure, status, outputShapes, timing);
            return false;
        });
    cb(status, outputShapes, timing);

    native_exec_.reset();
    return Void();
}

Return<void> VsiPreparedModel::executeFenced(
    const hal::Request& request,
    const hal::hidl_vec<hal::hidl_handle>& wait_for,
    hal::MeasureTiming measure,
    const hal::OptionalTimePoint& deadline,
    const hal::OptionalTimeoutDuration& loopTimeoutDuration,
    const hal::OptionalTimeoutDuration& duration,
    executeFenced_cb cb) {
    V1_3::ErrorStatus status;
    hidl_vec<OutputShape> outputShapes;
    Timing timing;

    // idle -> lock -> lock - > idle
    for (const auto& hal_op : model_.main.operations) {
        uint16_t extensionType = VsiDriver::findExtensionOperation(hal_op);

        if (isDeviceLocked) {
            if (VSI_NPU_LOCK == extensionType) {
                // lock - > lock
                cb(V1_3::ErrorStatus::DEVICE_UNAVAILABLE, hidl_handle(nullptr), nullptr);
                native_exec_.reset();
                return Void();
            } else if (VSI_NPU_UNLOCK == extensionType) {
                // lock -> unlock
                isDeviceLocked = false;
                cb(V1_3::ErrorStatus::NONE, hidl_handle(nullptr), nullptr);
                native_exec_.reset();
                return Void();
            }
        } else {  // device not locked
            if (VSI_NPU_LOCK == extensionType) {
                // unlock - > lock
                isDeviceLocked = true;
                cb(V1_3::ErrorStatus::NONE, hidl_handle(nullptr), nullptr);
                native_exec_.reset();
                return Void();
            } else if (VSI_NPU_UNLOCK == extensionType) {
                cb(V1_3::ErrorStatus::NONE, hidl_handle(nullptr), nullptr);
                native_exec_.reset();
                return Void();
            }
        }
    }

    // Wait for the dependent events to signal
    for (const auto& fenceHandle : wait_for) {
        if (!fenceHandle.getNativeHandle()) {
            cb(V1_3::ErrorStatus::INVALID_ARGUMENT, hidl_handle(nullptr), nullptr);
            return Void();
        }
        int syncFenceFd = fenceHandle.getNativeHandle()->data[0];
        if (syncWait(syncFenceFd, -1) != FenceState::SIGNALED) {
            LOG(ERROR) << "syncWait failed";
            cb(V1_3::ErrorStatus::GENERAL_FAILURE, hidl_handle(nullptr), nullptr);
            return Void();
        }
    }

    VsiPreparedModel::sandbox_.wait(
        [this, request, measure, &status, &outputShapes, &timing]() -> bool {
            this->executeBase_1_3(request, measure, status, outputShapes, timing);
            return false;
        });

    if (V1_3::ErrorStatus::NONE != status) {
        cb(status, hidl_handle(nullptr), nullptr);
        return Void();
    }
    Timing timingSinceLaunch;
    Timing timingAfterFence;
    if (MeasureTiming::YES == measure) {
        timingSinceLaunch = timing;
        timingAfterFence = timing;
    } else {
        timingSinceLaunch = kNoTiming;
        timingAfterFence = kNoTiming;
    }
    sp<VsiFencedExecutionCallback> fencedExecutionCallback =
            new VsiFencedExecutionCallback(timingSinceLaunch, timingAfterFence, status);
    cb(status, hidl_handle(nullptr), fencedExecutionCallback);

    native_exec_.reset();

    return Void();
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

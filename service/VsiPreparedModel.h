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
#ifndef ANDROID_ML_VSI_PREPAREMODEL_H
#define ANDROID_ML_VSI_PREPAREMODEL_H
#include <nnrt/compilation.hpp>
#include <nnrt/execution.hpp>
#include <nnrt/file_map_memory.hpp>
#include <nnrt/model.hpp>
#include <nnrt/op/public.hpp>
#include <nnrt/types.hpp>
#include <nnrt/error.hpp>
#include <vector>

#include "HalInterfaces.h"
#include "CpuExecutor.h"
#include "SandBox.h"
#include "Utils.h"
#include <sys/system_properties.h>
#include "VsiPlatform.h"
#include "VsiRTInfo.h"

#if ANDROID_SDK_VERSION >= 30
#include "BufferTracker.h"
#endif

using android::sp;

namespace android {
namespace nn {
namespace vsi_driver {

#if ANDROID_SDK_VERSION >= 29
using time_point = std::chrono::steady_clock::time_point;
static const Timing kNoTiming = {.timeOnDevice = UINT64_MAX, .timeInDriver = UINT64_MAX};
static std::atomic_bool isDeviceLocked = 0;
#endif

static inline auto now() {
    return std::chrono::steady_clock::now();
};

static inline auto microsecondsDuration(decltype(now()) end, decltype(now()) start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
};

#if ANDROID_SDK_VERSION >= 30
class VsiFencedExecutionCallback : public IFencedExecutionCallback {
   public:
    VsiFencedExecutionCallback(Timing timingSinceLaunch,
                               Timing timingAfterFence,
                               V1_3::ErrorStatus error)
        : kTimingSinceLaunch(timingSinceLaunch),
          kTimingAfterFence(timingAfterFence),
          kErrorStatus(error) {}
    hal::Return<void> getExecutionInfo(getExecutionInfo_cb callback) override {
        callback(kErrorStatus, kTimingSinceLaunch, kTimingAfterFence);
        return hal::Void();
    }

   private:
    const Timing kTimingSinceLaunch;
    const Timing kTimingAfterFence;
    const V1_3::ErrorStatus kErrorStatus;
};
#endif

class VsiPreparedModel : public HalPlatform::PrepareModel {
   public:
    VsiPreparedModel(const HalPlatform::Model& model,
                     ExecutionPreference preference,
                     int cacheHandle)
        : model_(model), preference_(preference) {
        native_model_ = std::make_shared<nnrt::Model>();
        int npu_cache_model = getSystemPropertyAsInt("npu.cache.model", 0);

        if (npu_cache_model && native_model_->set_cache_handle(cacheHandle)) {
            LOG(INFO) << "Cache setup Success";
        }
    }

    VsiPreparedModel(const HalPlatform::Model& model, ExecutionPreference preference)
        : model_(model), preference_(preference) {
        native_model_ = std::make_shared<nnrt::Model>();
    }

    ~VsiPreparedModel() override { release_rtinfo(const_buffer_); }

    /*map hidl model to ovxlib model and compliation*/
    Return<ErrorStatus> initialize();
    Return<ErrorStatus> initializeCacheInternel();
    template <typename T_IExecutionCallback>
    Return<ErrorStatus> executeBase(const Request& request, const T_IExecutionCallback& callback);

    Return<ErrorStatus> execute(const Request& request,
                                const sp<V1_0::IExecutionCallback>& callback) override;
#if ANDROID_SDK_VERSION >= 29
    Return<void> executeSynchronously(const Request& request,
                                      MeasureTiming measure,
                                      executeSynchronously_cb cb) override;

    Return<ErrorStatus> execute_1_2(const Request& request,
                                    MeasureTiming measure,
                                    const sp<V1_2::IExecutionCallback>& callback) override;

    Return<void> configureExecutionBurst(
        const sp<V1_2::IBurstCallback>& callback,
        const android::hardware::MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
        const android::hardware::MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
        configureExecutionBurst_cb cb) override;
#endif

#if ANDROID_SDK_VERSION >= 30
    Return<void> executeSynchronously_1_3(const V1_3::Request& request,
                                          MeasureTiming measure,
                                          const OptionalTimePoint& deadline,
                                          const OptionalTimeoutDuration& loopTimeoutDuration,
                                          executeSynchronously_1_3_cb cb) override;

    Return<V1_3::ErrorStatus> execute_1_3(const V1_3::Request& request,
                                          MeasureTiming measure,
                                          const OptionalTimePoint& deadline,
                                          const OptionalTimeoutDuration& loopTimeoutDuration,
                                          const sp<V1_3::IExecutionCallback>& callback) override;

    Return<void> executeFenced(const V1_3::Request& request,
                               const hidl_vec<hidl_handle>& wait_for,
                               MeasureTiming measure,
                               const OptionalTimePoint& deadline,
                               const OptionalTimeoutDuration& loopTimeoutDuration,
                               const OptionalTimeoutDuration& duration,
                               executeFenced_cb cb) override;

    const HalPlatform::Model* getModel() const { return &model_; }
#endif

   private:
    enum IO { INPUT = 0, OUTPUT };
#if ANDROID_SDK_VERSION < 29
    struct OutputShape {
        std::vector<uint32_t> dimensions;
        bool isSufficient;
    };
#endif
    void release_rtinfo(std::vector<VsiRTInfo>& rtInfos);

    void fill_operand_value(nnrt::op::OperandPtr ovx_operand,
                            const HalPlatform::Operand& hal_operand);

    Return<ErrorStatus> construct_ovx_operand(nnrt::op::OperandPtr ovx_oprand,
                                              const HalPlatform::Operand& hal_operand);

    Return<ErrorStatus> map_rtinfo_from_hidl_memory(const hidl_vec<hidl_memory>& pools,
                                                    std::vector<VsiRTInfo>& rtInfos);

    Return<ErrorStatus> update_pool_info_from_request(const Request& request,
                                                      const std::vector<uint32_t>& indexes,
                                                      const hidl_vec<RequestArgument>& arguments,
                                                      IO flag,
                                                      std::vector<OutputShape>& outputShapes);

    void update_operand_from_request(const std::vector<uint32_t>& indexes,
                                     const hidl_vec<RequestArgument>& arguments);

    Return<ErrorStatus> convertResultCodeToErrorStatus(int resultCode);
#if ANDROID_SDK_VERSION >= 29
    Return<ErrorStatus> executeBase(const Request& request,
                                    MeasureTiming measure,
                                    ErrorStatus& status,
                                    hidl_vec<OutputShape>& outputShapes,
                                    Timing& timing);

    static Return<void> notify(const sp<V1_0::IExecutionCallback>& callback,
                               const ErrorStatus& status,
                               const hidl_vec<OutputShape>&,
                               Timing);

    static Return<void> notify(const sp<V1_2::IExecutionCallback>& callback,
                               const ErrorStatus& status,
                               const hidl_vec<OutputShape>& outputShapes,
                               Timing timing);

    static void notify(const V1_2::IPreparedModel::executeSynchronously_cb& callback,
                       const ErrorStatus& status,
                       const hidl_vec<OutputShape>& outputShapes,
                       Timing timing);
#endif
#if ANDROID_SDK_VERSION >= 30
    Return<V1_3::ErrorStatus> mapRtinfoFromRequest(const V1_3::Request& request,
                                                              std::vector<VsiRTInfo>& rtInfos);

    Return<V1_3::ErrorStatus> updateDeivceMem(const V1_3::Request& request,
                                              const hidl_vec<OutputShape>& outputShapes);

    Return<V1_3::ErrorStatus> executeBase_1_3(const V1_3::Request& request,
                                              MeasureTiming measure,
                                              V1_3::ErrorStatus& oStatus,
                                              hidl_vec<OutputShape>& oOutputShapes,
                                              Timing& oTiming);
#endif
    template <typename T>
    struct NativeDeleter {
        void operator()(T* data) {
            VsiPreparedModel::sandbox_.wait(std::bind(&NativeDeleter::deleter, data));
        }

        static bool deleter(T* m) {
            delete m;
            return false;
        }
    };

    const HalPlatform::Model model_;
    ExecutionPreference preference_;
    std::shared_ptr<nnrt::Model> native_model_;
    std::shared_ptr<nnrt::Compilation> native_compile_;
    std::shared_ptr<nnrt::Execution> native_exec_;

    /*store pointer of all of hidl_memory to buffer*/
    std::vector<VsiRTInfo> const_buffer_;
    std::vector<VsiRTInfo> io_buffer_;

    static SandBox sandbox_;
};
}  // namespace vsi_driver
}  // namespace nn
}  // namespace android
#endif

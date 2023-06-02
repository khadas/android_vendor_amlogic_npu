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
#define LOG_TAG "VsiDevice"

#include "VsiDevice.h"
#include "VsiBuffer.h"

#include "HalInterfaces.h"
#include "ValidateHal.h"

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>
namespace android {
namespace nn {
namespace vsi_driver {

std::shared_ptr<BufferTracker> kBufferTracker = BufferTracker::create();

Return<V1_3::ErrorStatus> VsiDevice::prepareModel_1_3(
    const V1_3::Model& model,
    ExecutionPreference preference,
    hal::Priority priority,
    const OptionalTimePoint& deadline,
    const hidl_vec<hidl_handle>& modelCache,
    const hidl_vec<hidl_handle>& dataCache,
    const CacheToken& token,
    const sp<V1_3::IPreparedModelCallback>& callback) {
    if (VLOG_IS_ON(DRIVER)) {
        VLOG(DRIVER) << "prepareModel";
        logModelToInfo(model);
    }
    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model)) {
        LOG(ERROR) << "invalid hal model";
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT, nullptr);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateExecutionPreference(preference)) {
        LOG(ERROR) << "invalid preference" << static_cast<int32_t>(preference);
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT, nullptr);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    if (!validatePriority(priority)) {
        LOG(ERROR) << "invalid priority";
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT, nullptr);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    int handle = -1;
    if (modelCache.size() == 1 && dataCache.size() == 1 && modelCache[0]->numFds == 1 &&
        dataCache[0]->numFds == 1) {
        handle = modelCache[0]->data[0];
        if (handle == -1) {
            LOG(ERROR) << "PrepareModel_1_3 get error cache handle.";
            return V1_3::ErrorStatus::GENERAL_FAILURE;
        }
    }

    sp<VsiPreparedModel> preparedModel = new VsiPreparedModel(model, preference, handle);
    std::thread(asyncPrepareModel_1_3<sp<IPreparedModelCallback>>, preparedModel, callback)
        .detach();

    return V1_3::ErrorStatus::NONE;
}

static const VsiPreparedModel* castToVsiPreparedModel(
    const sp<V1_3::IPreparedModel>& preparedModel) {
    if (preparedModel->isRemote()) {
        return nullptr;
    } else {
        return static_cast<const VsiPreparedModel*>(preparedModel.get());
    }
}

Return<void> VsiDevice::allocate(const V1_3::BufferDesc& desc,
                                 const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
                                 const hidl_vec<V1_3::BufferRole>& inputRoles,
                                 const hidl_vec<V1_3::BufferRole>& outputRoles,
                                 V1_3::IDevice::allocate_cb cb) {
    constexpr uint32_t kInvalidBufferToken = 0;

    VLOG(DRIVER) << "VsiDevice::allocate";
    std::set<PreparedModelRole> roles;
    V1_3::Operand operand;
    auto getModel = [](const sp<V1_3::IPreparedModel>& preparedModel) -> const V1_3::Model* {
        const auto* vsiPreparedModel = castToVsiPreparedModel(preparedModel);
        if (vsiPreparedModel == nullptr) {
            LOG(ERROR) << "VsiDevice::allocate -- unknown remote IPreparedModel.";
            return nullptr;
        }
        return vsiPreparedModel->getModel();
    };
    if (!validateMemoryDesc(desc, preparedModels, inputRoles, outputRoles, getModel, &roles,
                            &operand)) {
        LOG(ERROR) << "VsiDevice::allocate -- validation failed.";
        cb(V1_3::ErrorStatus::INVALID_ARGUMENT, nullptr, kInvalidBufferToken);
        return Void();
    }

    if (isExtensionOperandType(operand.type)) {
        LOG(ERROR) << "VsiDevice::allocate -- does not support extension type.";
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return Void();
    }

    uint32_t size = nonExtensionOperandSizeOfData(operand.type, operand.dimensions);
    VLOG(DRIVER) << "VsiDevice::allocate -- type = " << toString(operand.type)
                 << ", dimensions = " << toString(operand.dimensions) << ", size = " << size;
    if (size == 0) {
        LOG(ERROR) << "VsiDevice::allocate -- does not support dynamic output shape.";
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return Void();
    }

    auto bufferWrapper = ManagedBuffer::create(size, std::move(roles), std::move(operand));
    if (bufferWrapper == nullptr) {
        LOG(ERROR) << "VsiDevice::allocate -- not enough memory.";
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return Void();
    }

    auto token = kBufferTracker->add(bufferWrapper);
    if (token == nullptr) {
        LOG(ERROR) << "VsiDevice::allocate -- BufferTracker returned invalid token.";
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return Void();
    }

    const uint32_t tokenValue = token->get();
    sp<VsiBuffer> vsiBuffer = new VsiBuffer(std::move(bufferWrapper), std::move(token));
    VLOG(DRIVER) << "VsiDevice::allocate -- successfully allocates the requested memory";
    cb(V1_3::ErrorStatus::NONE, std::move(vsiBuffer), tokenValue);
    return Void();
};

Return<V1_3::ErrorStatus> VsiDevice::prepareModelFromCache_1_3(
    const OptionalTimePoint& deadline,
    const hidl_vec<hidl_handle>& modelCache,
    const hidl_vec<hidl_handle>& dataCache,
    const CacheToken& token,
    const sp<V1_3::IPreparedModelCallback>& callback) {
    notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, nullptr);
    return V1_3::ErrorStatus::GENERAL_FAILURE;
}
}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

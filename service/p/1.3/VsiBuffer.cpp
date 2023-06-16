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

#include "VsiBuffer.h"
#include "CpuExecutor.h"

namespace android {
namespace nn {
namespace vsi_driver {

Return<V1_3::ErrorStatus> VsiBuffer::copyTo(const hidl_memory& dst) {
    const auto dstPool = RunTimePoolInfo::createFromHidlMemory(dst);
    if (!dstPool.has_value()) {
        LOG(ERROR) << "VsiBuffer::copyTo -- unable to map dst memory.";
        return V1_3::ErrorStatus::GENERAL_FAILURE;
    }
    auto validationStatus = kBuffer->validateCopyTo(dstPool->getSize());
    auto status_1_3 = HalPlatform::convertVersion(validationStatus);
    if (status_1_3 != V1_3::ErrorStatus::NONE) {
        return status_1_3;
    }
    const auto srcPool = kBuffer->createRunTimePoolInfo();
    auto dstRtPoolInfo = dstPool.value();
    CHECK(srcPool.getBuffer() != nullptr);
    CHECK(dstRtPoolInfo.getBuffer() != nullptr);
    CHECK(srcPool.getSize() == dstRtPoolInfo.getSize());
    std::copy(srcPool.getBuffer(), srcPool.getBuffer() + srcPool.getSize(), dstRtPoolInfo.getBuffer());
    dstRtPoolInfo.flush();
    return V1_3::ErrorStatus::NONE;
}

static V1_3::ErrorStatus CopyFromImpl(const hidl_memory& src,
                                    const hidl_vec<uint32_t>& dimensions,
                                    const std::shared_ptr<ManagedBuffer>& bufferWrapper) {
    CHECK(bufferWrapper != nullptr);
    const auto srcPool = RunTimePoolInfo::createFromHidlMemory(src);
    if (!srcPool.has_value()) {
        LOG(ERROR) << "VsiBuffer::copyFrom -- unable to map src memory.";
        return V1_3::ErrorStatus::GENERAL_FAILURE;
    }
    auto validationStatus =
        bufferWrapper->validateCopyFrom(dimensions, srcPool->getSize());
    auto status_1_3 = HalPlatform::convertVersion(validationStatus);
    if (status_1_3 != V1_3::ErrorStatus::NONE) {
        return status_1_3;
    }
    const auto dstPool = bufferWrapper->createRunTimePoolInfo();
    auto srcRtPoolInfo = srcPool.value();
    CHECK(srcRtPoolInfo.getBuffer() != nullptr);
    CHECK(dstPool.getBuffer() != nullptr);
    CHECK(srcRtPoolInfo.getSize() == dstPool.getSize());
    std::copy(srcRtPoolInfo.getBuffer(), srcRtPoolInfo.getBuffer() + srcRtPoolInfo.getSize(), dstPool.getBuffer());
    dstPool.flush();
    return V1_3::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> VsiBuffer::copyFrom(const hidl_memory& src,
                                           const hidl_vec<uint32_t>& dimensions) {
    const auto status = CopyFromImpl(src, dimensions, kBuffer);
    if (status == V1_3::ErrorStatus::NONE) {
        kBuffer->updateDimensions(dimensions);
        kBuffer->setInitialized(true);
    } else {
        kBuffer->setInitialized(false);
    }
    return status;
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android
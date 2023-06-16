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

#ifndef ANDROID_ML_NN_VSI_DEVICE_1_3_BUFFER_H
#define ANDROID_ML_NN_VSI_DEVICE_1_3_BUFFER_H

#include "VsiDevice.h"

namespace android {
namespace nn {
namespace vsi_driver {

// Manages the data buffer for an operand.
class VsiBuffer : public IBuffer {
   public:
    #if ANDROID_SDK_VERSION >= 31
    VsiBuffer(std::shared_ptr<HalManagedBuffer> buffer, std::unique_ptr<HalBufferTracker::Token> token)
    #else
    VsiBuffer(std::shared_ptr<ManagedBuffer> buffer, std::unique_ptr<BufferTracker::Token> token)
    #endif
        : kBuffer(std::move(buffer)), kToken(std::move(token)) {
        CHECK(kBuffer != nullptr);
        CHECK(kToken != nullptr);
    }
    Return<V1_3::ErrorStatus> copyTo(const hidl_memory& dst) override;
    Return<V1_3::ErrorStatus> copyFrom(const hidl_memory& src,
                                           const hidl_vec<uint32_t>& dimensions) override;

   private:
    #if ANDROID_SDK_VERSION >= 31
    const std::shared_ptr<HalManagedBuffer> kBuffer;
    const std::unique_ptr<HalBufferTracker::Token> kToken;
    #else
    const std::shared_ptr<ManagedBuffer> kBuffer;
    const std::unique_ptr<BufferTracker::Token> kToken;
    #endif
};

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

#endif
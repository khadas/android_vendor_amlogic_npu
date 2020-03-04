/****************************************************************************
*
*    Copyright (c) 2019 Vivante Corporation
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
#ifndef ANDROID_ML_NN_COMMON_OPENVX_EXECUTOR_H
#define ANDROID_ML_NN_COMMON_OPENVX_EXECUTOR_H
#include <vector>

#include "model.hpp"
#include "types.hpp"
#include "compilation.hpp"
#include "execution.hpp"
#include "op/public.hpp"
#include "file_map_memory.hpp"
#include "VsiDriver.h"
#include "HalInterfaces.h"

#include "Utils.h"
using android::sp;

namespace android {
namespace nn {
namespace vsi_driver {
    class VsiPreparedModel : public V1_0::IPreparedModel {
   public:
    VsiPreparedModel(const V1_1::Model& model):model_(model) {
        native_model_ = std::make_shared<nnrt::Model>();
        Create(model);
        }

    ~VsiPreparedModel() override {
        release_rtinfo(const_buffer_);
        }

    // TODO: Make this asynchronous
    Return<ErrorStatus> execute(
        const Request& request,
        const sp<V1_0::IExecutionCallback>& callback) override;

   private:
        /*create ovxlib model and compliation*/
        Return<ErrorStatus> Create(const V1_1::Model& model);

        void fill_operand_value(nnrt::op::OperandPtr ovx_operand, const V1_0::Operand& hal_operand) ;
        void construct_ovx_operand(nnrt::op::OperandPtr ovx_oprand,const V1_0::Operand& hal_operand);
        int map_rtinfo_from_hidl_memory(const hidl_vec<hidl_memory>& pools,
            std::vector<VsiRTInfo>& rtInfos);
        void release_rtinfo(std::vector<VsiRTInfo>& rtInfos);

        const V1_1::Model model_;
        std::shared_ptr<nnrt::Model> native_model_;
        std::shared_ptr<nnrt::Compilation> native_compile_;
        std::shared_ptr<nnrt::Execution> native_exec_;

        /*store pointer of all of hidl_memory to buffer*/
        std::vector<VsiRTInfo> const_buffer_;
        std::vector<VsiRTInfo> io_buffer_;
};
}
}
}
#endif

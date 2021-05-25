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

#ifndef _STRIDED_SLICE_VALIDATE_HPP_
#define _STRIDED_SLICE_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class StridedSliceValidate : public OperationValidate<T_model, T_Operation> {
   public:
    StridedSliceValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}

    bool IsSplitOnBatch(std::string& reason) {
        bool isSupport = true;
        {
            const uint32_t input = 0;
            const uint32_t operand_begin = 1;
            const uint32_t operand_end = 2;
            const uint32_t operand_stride = 3;

            auto input_rank =
                this->m_Model.operands[this->m_Operation.inputs[input]].dimensions.size();
            auto input_batch =
                this->m_Model.operands[this->m_Operation.inputs[input]].dimensions[0];

            struct vsi_driver::VsiRTInfo rt;

            auto begin_vec = reinterpret_cast<const int32_t*>(get_buffer::getOperandDataPtr(
                this->m_Model,
                this->m_Model.operands[this->m_Operation.inputs[operand_begin]],
                rt));
            auto end_vec = reinterpret_cast<const int32_t*>(get_buffer::getOperandDataPtr(
                this->m_Model, this->m_Model.operands[this->m_Operation.inputs[operand_end]], rt));
            auto stride_vec = reinterpret_cast<const int32_t*>(get_buffer::getOperandDataPtr(
                this->m_Model,
                this->m_Model.operands[this->m_Operation.inputs[operand_stride]],
                rt));

            bool isSplitNotOnBatch =
                (begin_vec[0] == 0) && (end_vec[0] == input_batch) && (stride_vec[0] == 1);
            LOG(INFO) << "begin[o] = " << begin_vec[0] << ", end[0] = " << end_vec[0]
                      << ", input_rank = " << input_rank << ", Input_Batch = " << input_batch
                      << ", stride[0] = " << stride_vec[0];

            if (input_rank == 4 && !isSplitNotOnBatch) {
                reason +=
                    "Reject StrideSlice because split on Batch which we don't handled it with SW";
                isSupport &= false;
            }
        }

        return isSupport;
    }

    bool SignatureCheck(std::string& reason) override {
        if (::hal::limitation::nnapi::match("StridedSlice_Inputs", this->InputArgTypes()) &&
            ::hal::limitation::nnapi::match("StridedSlice_Outputs", this->OutputArgTypes())) {
            bool is_support = IsSplitOnBatch(reason);
            for (auto input_idx = 1; is_support && (input_idx < this->m_Operation.inputs.size()); ++input_idx) {
                if (this->IsInput(input_idx)) {
                    is_support = false;
                    reason +=
                        "StridedSlice: not supported because only input(0) can be model_input";
                    break;
                }
            }

            return is_support;
        }
        else {
            reason += "StridedSlice: signature matching failed";
            return false;
        }
    }
};

}  // namespace op_validate
}  // namespace nn
}  // namespace android
#endif
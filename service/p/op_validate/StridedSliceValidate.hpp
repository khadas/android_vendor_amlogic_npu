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

    bool IsValidParam(std::string& reason) {
        bool isSupport = true;
        {
            auto operation = this->OperationForRead();
            const uint32_t operand_input = operation.inputs[0];
            const uint32_t operand_begin = operation.inputs[1];
            const uint32_t operand_end = operation.inputs[2];
            const uint32_t operand_stride = operation.inputs[3];
            const uint32_t operand_begin_mask = operation.inputs[4];
            const uint32_t operand_end_mask = operation.inputs[5];
            const uint32_t operand_shrink_axis_mask = operation.inputs[6];

            auto input_dims = get_buffer::getOpeand(this->m_Model, operand_input).dimensions;
            auto begin_dims = get_buffer::getOpeand(this->m_Model, operand_begin).dimensions;
            auto end_dims = get_buffer::getOpeand(this->m_Model, operand_end).dimensions;
            auto stride_dims = get_buffer::getOpeand(this->m_Model, operand_stride).dimensions;

            if (begin_dims.size() != 1 || end_dims.size() != 1 || stride_dims.size() != 1 ||
                begin_dims[0] != input_dims.size() || end_dims[0] != input_dims.size() ||
                stride_dims[0] != input_dims.size()) {
                reason += "Reject StrideSlice, becasue of invalid parameter";
                return false;
            }

            struct vsi_driver::VsiRTInfo begin_rt;
            auto begin_vec = reinterpret_cast<const int32_t*>(get_buffer::getOperandDataPtr(
                this->m_Model, get_buffer::getOpeand(this->m_Model, operand_begin), begin_rt));

            struct vsi_driver::VsiRTInfo end_rt;
            auto end_vec = reinterpret_cast<const int32_t*>(get_buffer::getOperandDataPtr(
                this->m_Model, get_buffer::getOpeand(this->m_Model, operand_end), end_rt));

            struct vsi_driver::VsiRTInfo stride_rt;
            auto stride_vec = reinterpret_cast<const int32_t*>(get_buffer::getOperandDataPtr(
                this->m_Model, get_buffer::getOpeand(this->m_Model, operand_stride), stride_rt));

            const int32_t begin_mask = get_buffer::getScalarData<int32_t>(
                this->m_Model, get_buffer::getOpeand(this->m_Model, operand_begin_mask));
            const int32_t end_mask = get_buffer::getScalarData<int32_t>(
                this->m_Model, get_buffer::getOpeand(this->m_Model, operand_end_mask));
            const int32_t shrink_axis_mask = get_buffer::getScalarData<int32_t>(
                this->m_Model, get_buffer::getOpeand(this->m_Model, operand_shrink_axis_mask));

            for (int32_t idx = 0; idx < input_dims.size(); idx++) {
                int32_t begin = (begin_mask & (1 << idx)) ? 0 : begin_vec[idx];
                int32_t end = (end_mask & (1 << idx)) ? input_dims.size() - 1 : end_vec[idx];
                int32_t stride = stride_vec[idx];

                if (stride <= 0) {
                    reason += "StridedSilce: not supported stride parameter less than 0";
                    return false;
                }

                int32_t outDim = ceil((end - begin) / static_cast<float>(stride));

                if ((shrink_axis_mask & (1 << idx)) && outDim != 1) {
                    reason += "StridedSilce: invalid input paramter, because the " +
                              std::to_string(idx) + "th bit of shrink_axis_mask is set to 1," +
                              "please check parameter $begin and $end ";
                    return false;
                }
            }
            auto input_rank = get_buffer::getOpeand(this->m_Model, operand_input).dimensions.size();
            auto input_batch = get_buffer::getOpeand(this->m_Model, operand_input).dimensions[0];

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
            auto operation = this->OperationForRead();
            for (auto input_idx = 1; input_idx < operation.inputs.size(); ++input_idx) {
                if (!this->IsConstantTensor(operation.inputs[input_idx])) {
                    reason +=
                        "StridedSlice: not supported because only input(0) can be model_input";
                    return false;
                }
            }

            return IsValidParam(reason);
        } else {
            reason += "StridedSlice: signature matching failed";
            return false;
        }
    }
};

}  // namespace op_validate
}  // namespace nn
}  // namespace android
#endif
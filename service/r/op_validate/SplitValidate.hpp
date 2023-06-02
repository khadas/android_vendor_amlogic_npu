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

#ifndef _SPLIT_VALIDATE_HPP_
#define _SPLIT_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class SplitValidate : public OperationValidate<T_model, T_Operation> {
   public:
    SplitValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        auto inputList = ::hal::limitation::nnapi::match("SplitInput", this->InputArgTypes());
        if (inputList) {
            // The outputs of split is stochastic, we only check inputs
            auto model = this->ModelForRead();
            auto op = this->OperationForRead();
            int32_t input_idx = inputList->ArgPos("input");
            int32_t axis_idx = inputList->ArgPos("axis");
            int32_t split_num_idx = inputList->ArgPos("split_number");
            auto input_operand = vsi_driver::GetHalOperand(model, input_idx);
            auto axis_num_operand = vsi_driver::GetHalOperand(model, axis_idx);
            auto split_num_operand = vsi_driver::GetHalOperand(model, split_num_idx);

            int32_t axis_num = get_buffer::getScalarData<int32_t>(model, axis_num_operand);
            int32_t split_num = get_buffer::getScalarData<int32_t>(model, split_num_operand);
            int32_t input_rank = input_operand.dimensions.size();
            int32_t batch_size = input_rank == 4 ? input_operand.dimensions[0] : 0;

            if (4 == input_rank && 0 != axis_num && batch_size * split_num * 2 > 2047) {
                reason += "Split exceed limitation";
                return false;
            }
        } else {
            reason += "Split invalid Signature";
            return false;
        }
        return true;
    }
};

}  // namespace op_validate
}  // namespace nn
}  // namespace android

#endif
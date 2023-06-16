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

#ifndef _MAXIMUM_MINIMUM_VALIDATE_HPP_
#define _MAXIMUM_MINIMUM_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class MaximumMinimumValidate : public OperationValidate<T_model, T_Operation> {
   public:
    MaximumMinimumValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        auto input_list = ::hal::limitation::nnapi::match("MaximumMinimumInput", this->InputArgTypes());
        auto output_list = ::hal::limitation::nnapi::match("MaximumMinimumOutput", this->OutputArgTypes());
        if (input_list && output_list) {
            auto operation = this->OperationForRead();
            auto input0 = operation.inputs[input_list->ArgPos("input0")];
            auto input1 = operation.inputs[input_list->ArgPos("input1")];
            if (this->IsConstantTensor(input0) && this->IsConstantTensor(input1)) {
                reason += "Reject Maximum/Minimum, because all input as constant not support.";
                return false;
            }
            return true;
        } else {
            reason += "Reject Maximum/Minimum, because input/ouput data type not support.";
            return false;
        }
    };
};

}  // end of op_validate
}
}

#endif
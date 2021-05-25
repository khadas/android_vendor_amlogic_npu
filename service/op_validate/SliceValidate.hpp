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

#ifndef _SLICE_VALIDATE_HPP_
#define _SLICE_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class SliceValidate : public OperationValidate<T_model, T_Operation> {
   public:
    SliceValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {};
    bool SignatureCheck(std::string& reason) override {
        auto model = this->ModelForRead();
        auto operation = this->OperationForRead();
        auto inputList = ::hal::limitation::nnapi::match("SliceInput", this->InputArgTypes());
        auto outputList = ::hal::limitation::nnapi::match("SliceOutput", this->OutputArgTypes());
        if (inputList && outputList) {
            auto& beginOperand = model.operands[operation.inputs[inputList->ArgPos("beginning")]];
            auto& sizeOperand = model.operands[operation.inputs[inputList->ArgPos("size")]];
            if (OperandLifeTime::MODEL_INPUT == beginOperand.lifetime ||
                OperandLifeTime::MODEL_INPUT == sizeOperand.lifetime) {
                reason += "reject SLICE because not support beginning or size as model input\n";
                return false;
            }
            auto& inputOperand = model.operands[operation.inputs[inputList->ArgPos("input")]];
            auto& outputOperand = model.operands[operation.outputs[0]];
            if (inputOperand.dimensions[0] != outputOperand.dimensions[0]) {
                reason += "reject SLICE because not support slice on batch\n";
                return false;
            }
            return true;
        } else {
            reason += "reject SLICE because input data type not support\n";
            return false;
        }
    };
};

}  // end of op_validate
}
}

#endif

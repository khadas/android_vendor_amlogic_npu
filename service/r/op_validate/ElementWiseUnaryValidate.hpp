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

#ifndef _ELEMENTWISE_UNARY_VALIDATE_HPP_
#define _ELEMENTWISE_UNARY_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class ElementWiseUnary : public OperationValidate<T_model, T_Operation> {
   public:
    ElementWiseUnary(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}

    bool SignatureCheck(std::string& reason) override {
        auto input_signature = GetInputSig();
        auto output_signature = GetOutputSig();

        auto inputList = ::hal::limitation::nnapi::match(input_signature, this->InputArgTypes());
        auto outputList = ::hal::limitation::nnapi::match(output_signature, this->OutputArgTypes());

        if (inputList && outputList) {  // signature matched
            auto inIdx = inputList->ArgPos("input");
            auto input_shape = vsi_driver::GetHalOperand(this->m_Model, inIdx).dimensions;
            if (this->IsTensor(vsi_driver::GetHalOperand(this->m_Model, inIdx)) &&
                input_shape.size() > 4) {
                reason += "reject";
                reason += GetOpName();
                reason += " because input shape rank > 4\n";
                return false;
            }

            return true;
        }

        reason += "Elementwise unary signature match failed\n";
        return false;
    };

    virtual std::string GetInputSig() = 0;
    virtual std::string GetOutputSig() = 0;
    virtual std::string GetOpName() = 0;
};
}  // namespace op_validate
}  // namespace nn
}  // namespace android
#endif

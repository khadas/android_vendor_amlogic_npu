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

#ifndef _NEG_VALIDATE_HPP_
#define _NEG_VALIDATE_HPP_

#include "ElementWiseUnaryValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {

template <typename T_Model, typename T_Operation>
class NegValidate : public ElementWiseUnary<T_Model, T_Operation> {
    using ElementWiseUnary<T_Model, T_Operation>::ElementWiseUnary;
    std::string GetInputSig() override { return std::string("NegInput"); }
    std::string GetOutputSig() override { return std::string("NegOutput"); }
    std::string GetOpName() override { return std::string("neg"); }
};

}  // namespace op_validate
}  // namespace nn
}  // namespace android

#endif
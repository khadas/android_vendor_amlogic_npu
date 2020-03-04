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

#ifndef _OPERATION_VALIDATW_H_
#define _OPERATION_VALIDATW_H_

#include <android-base/logging.h>
#include <vector>
#include "HalInterfaces.h"
#include "hal_limitation/nnapi_limitation.hpp"
#include "nnrt/types.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_Model, typename T_Operation>
class OperationValidate {
   public:
    OperationValidate(const T_Model& model, const T_Operation& operation)
        : m_Model(model), m_Operation(operation) {
        // GenInputArgTypes
        for (auto inIdx : m_Operation.inputs) {
            m_InputArgTypes.push_back(MapToNnrtOperandType(m_Model.operands[inIdx].type));
        }
        // GenOutputArgTypes
        for (auto inIdx : m_Operation.inputs) {
            m_OutputArgTypes.push_back(MapToNnrtOperandType(m_Model.operands[inIdx].type));
        }
    };
    virtual ~OperationValidate(){};

    bool IsDynamicShape() {
        for (auto outIdx : m_Operation.outputs) {
            auto& dims = m_Model.operands[outIdx].dimensions;
            for (auto dim : dims) {
                if (dim == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    // Default implementation
    virtual bool SignatureCheck() { return true; };

    virtual bool Validate() {
        bool isSupport = true;
        isSupport &= !IsDynamicShape();
        isSupport &= SignatureCheck();
        return isSupport;
    };

    bool IsConstantTensor(size_t index) {
        auto& operand = m_Model.operands[index];
        return operand.lifetime == OperandLifeTime::CONSTANT_COPY ||
               operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE;
    }

    T_Model m_Model;
    T_Operation m_Operation;
    std::vector<nnrt::OperandType> m_InputArgTypes;
    std::vector<nnrt::OperandType> m_OutputArgTypes;

   private:
    nnrt::OperandType MapToNnrtOperandType(OperandType type) {
        switch (type) {
            case OperandType::BOOL:
                return nnrt::OperandType::BOOL;
            case OperandType::INT32:
                return nnrt::OperandType::INT32;
            case OperandType::UINT32:
                return nnrt::OperandType::UINT32;
            case OperandType::FLOAT16:
                return nnrt::OperandType::FLOAT16;
            case OperandType::FLOAT32:
                return nnrt::OperandType::FLOAT32;
            case OperandType::TENSOR_BOOL8:
                return nnrt::OperandType::TENSOR_BOOL8;
            case OperandType::TENSOR_FLOAT16:
                return nnrt::OperandType::TENSOR_FLOAT16;
            case OperandType::TENSOR_FLOAT32:
                return nnrt::OperandType::TENSOR_FLOAT32;
            case OperandType::TENSOR_INT32:
                return nnrt::OperandType::TENSOR_INT32;
            case OperandType::TENSOR_QUANT8_ASYMM:
                return nnrt::OperandType::TENSOR_QUANT8_ASYMM;
            case OperandType::TENSOR_QUANT8_SYMM:
                return nnrt::OperandType::TENSOR_QUANT8_SYMM;
            case OperandType::TENSOR_QUANT16_ASYMM:
                return nnrt::OperandType::TENSOR_QUANT16_ASYMM;
            case OperandType::TENSOR_QUANT16_SYMM:
                return nnrt::OperandType::TENSOR_QUANT16_SYMM;
            case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
                return nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL;
            default:
                return nnrt::OperandType::NONE;
        }
    };
};

}  // end of op_validate
}
}

#endif
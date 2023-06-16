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

#ifndef _OPERATION_VALIDATW_H_
#define _OPERATION_VALIDATW_H_

#include <android-base/logging.h>
#include <vector>
#include "HalInterfaces.h"
#include "VsiPlatform.h"
#include "VsiRTInfo.h"
#include "hal_limitation/nnapi_limitation.hpp"
#include "nnrt/types.hpp"

namespace android {
namespace nn {
namespace op_validate {
using HalPlatform = vsi_driver::HalPlatform;
using OperationType = vsi_driver::OperationType;
using OperandType = vsi_driver::OperandType;
using OperandLifeTime = vsi_driver::OperandLifeTime;

namespace get_buffer {
const uint8_t* getOperandDataPtr(const HalPlatform::Model& model,
                                 const HalPlatform::Operand& halOperand,
                                 vsi_driver::VsiRTInfo& vsiMemory) {
    if (OperandLifeTime::CONSTANT_COPY == halOperand.lifetime) {
        return model.operandValues.data() + halOperand.location.offset;
    } else if (OperandLifeTime::CONSTANT_REFERENCE == halOperand.lifetime) {
        if (!mapHidlMem(model.pools[halOperand.location.poolIndex], vsiMemory)) return nullptr;
        return vsiMemory.getPtr(halOperand.location.offset);
    }
    return nullptr;
}

const uint8_t* getOperandPtr(const HalPlatform::Model& model,
                            const HalPlatform::Operand& operand,
                            struct vsi_driver::VsiRTInfo& rt) {
    auto& location = operand.location;
    if (operand.lifetime == OperandLifeTime::CONSTANT_COPY) {
        return model.operandValues.data() + location.offset;
    } else if (operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE) {
        return getOperandDataPtr(model, operand, rt);
    } else
        return nullptr;
}

template <typename T_type>
T_type getScalarData(const HalPlatform::Model& model, const HalPlatform::Operand& operand) {
    struct vsi_driver::VsiRTInfo rt;
    auto ptr = getOperandPtr(model, operand, rt);
    if (ptr)
        return *reinterpret_cast<T_type*>(const_cast<uint8_t*>(ptr));
    else
        return 0;
}

const HalPlatform::Operand& getOpeand(const HalPlatform::Model& model,
                            const uint32_t index) {
#if ANDROID_SDK_VERSION < 30
    if(index > model.operands.size()){
        LOG(ERROR)<< index << "is out of operands size :" << model.operands.size();
        assert(0);
    }
#elif ANDROID_SDK_VERSION >= 30
    if(index > model.main.operands.size()){
        LOG(ERROR)<< index << "is out of operands size :" << model.main.operands.size();
        assert(0);
    }
#endif
    return vsi_driver::GetHalOperand(model, index);
}
}  // end of get_buffer

template <typename T_Model, typename T_Operation>
class OperationValidate {
   public:
    OperationValidate(const T_Model& model, const T_Operation& operation)
        : m_Model(model), m_Operation(operation) {
        // GenInputArgTypes
        for (auto inIdx : m_Operation.inputs) {
            m_InputArgTypes.push_back(MapToNnrtOperandType(vsi_driver::GetHalOperand(m_Model, inIdx).type));
        }
        // GenOutputArgTypes
        // push first input into output argtypes
        m_OutputArgTypes.push_back(
            MapToNnrtOperandType(vsi_driver::GetHalOperand(m_Model, m_Operation.inputs[0]).type));
        for (auto outIdx : m_Operation.outputs) {
            m_OutputArgTypes.push_back(MapToNnrtOperandType(vsi_driver::GetHalOperand(m_Model, outIdx).type));
        }
    };
    virtual ~OperationValidate(){};

    virtual bool Validate(std::string& reason) {
        bool isSupport = DimentionCheck(reason) && ConstantTensorCheck(reason) &&
                         ShareOperandCheck(reason) && SignatureCheck(reason);
        return isSupport;
    };

    const std::vector<nnrt::OperandType>& InputArgTypes() const { return m_InputArgTypes; }
    const std::vector<nnrt::OperandType>& OutputArgTypes() const { return m_OutputArgTypes; }
    const T_Model& ModelForRead() const { return m_Model; }
    const T_Operation& OperationForRead() const { return m_Operation; }

    bool IsConstantTensor(size_t index) {
        auto& operand = vsi_driver::GetHalOperand(m_Model, index);
        return operand.lifetime == OperandLifeTime::CONSTANT_COPY ||
               operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE;
    }

    bool IsInput(size_t index) {
        auto& operand =vsi_driver::GetHalOperand(m_Model, index);
#if ANDROID_SDK_VERSION < 30
        return operand.lifetime == OperandLifeTime::MODEL_INPUT;
#elif ANDROID_SDK_VERSION >=30
        return operand.lifetime == OperandLifeTime::SUBGRAPH_INPUT;
#endif
    }

   protected:
    // Default implementation
    virtual bool SignatureCheck(std::string& reason) { return true; };

    bool IsTensor(const HalPlatform::Operand& operand) {
        bool tensor = true;
        switch (operand.type) {
            case OperandType::BOOL:
            case OperandType::FLOAT16:
            case OperandType::FLOAT32:
            case OperandType::INT32:
            case OperandType::UINT32:
                tensor = false;
                break;
            default:
                tensor = true;
        }
        return tensor;
    }

    bool DimentionCheck(std::string& reason) {
        // Check inputs
        if (0 == m_Operation.inputs.size()) return false;
        for (auto inIdx : m_Operation.inputs) {
            auto& dims = vsi_driver::GetHalOperand(m_Model, inIdx).dimensions;
            if (IsTensor(vsi_driver::GetHalOperand(m_Model, inIdx)) && dims.size() == 0) {
                reason += "reject op because its input tensor rank == 0\n";
                return false;
            }
            if (dims.size() > 6) {
                reason += "reject op because its input rank > 6\n";
                return false;
            }
            for (auto dim : dims) {
                if (dim == 0) {
                    return false;
                }
            }
        }
        // Check outputs
        if (0 == m_Operation.outputs.size()) return false;
        for (auto outIdx : m_Operation.outputs) {
            auto& dims = vsi_driver::GetHalOperand(m_Model, outIdx).dimensions;
            if (IsTensor(vsi_driver::GetHalOperand(m_Model, outIdx)) && dims.size() == 0) {
                reason += "reject op because its output tensor rank == 0\n";
                return false;
            }
            if (dims.size() > 6) {
                reason += "reject op because its output rank > 6\n";
                return false;
            }
            for (auto dim : dims) {
                if (dim == 0) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * @brief shared operand for single operation is not strongly supported, reject them
     *
     * @param reason promote message for the reason
     * @return true
     * @return false
     */
    bool ShareOperandCheck(std::string& reason) {
        size_t numOfOperand = m_Operation.inputs.size();
        std::set< uint32_t > uniqueOperands;
        std::for_each(m_Operation.inputs.begin(), m_Operation.inputs.end(), [&uniqueOperands](const uint32_t& id){
            uniqueOperands.insert(id);
        });

        if (uniqueOperands.size() != numOfOperand) {
            reason += "reject operation because its inputs shared same opearnd\n";
            return false;
        }
        return true;
    }

    bool ConstantTensorCheck(std::string& reason) {
        std::vector<OperationType> whiteList = {OperationType::ADD,
                                                OperationType::SUB,
                                                OperationType::MUL,
                                                OperationType::DIV,
                                                OperationType::MAXIMUM,
                                                OperationType::MINIMUM,
                                                OperationType::CONCATENATION,
                                                OperationType::CONV_2D,
                                                OperationType::FULLY_CONNECTED,
                                                OperationType::LSTM,
                                                OperationType::DEPTHWISE_CONV_2D};

        if (std::find(whiteList.begin(), whiteList.end(), m_Operation.type) == whiteList.end()) {
            if (IsConstantTensor(m_Operation.inputs[0])) {
                reason += "reject operation due to its input[0] is contant tensor which is meaningless\n";
                return false;
            }
        }
        return true;
    }

    T_Model ModelForWrite() { return m_Model; }
    T_Operation OperationForWrite() { return m_Operation; }

   protected:
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
            #if ANDROID_SDK_VERSION >= 30
            case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
                return nnrt::OperandType::TENSOR_QUANT8_ASYMM_SIGNED;
            #endif
            default:
                return nnrt::OperandType::NONE;
        }
    }

    protected:
    T_Model m_Model;
    T_Operation m_Operation;
    std::vector<nnrt::OperandType> m_InputArgTypes;
    std::vector<nnrt::OperandType> m_OutputArgTypes;
};

}  // end of op_validate
}
}

#endif

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

#ifndef _SPACE_OP_VALIDATE_HPP_
#define _SPACE_OP_VALIDATE_HPP_

#include "OperationValidate.hpp"
#include <iostream>

namespace android {
namespace nn {
namespace op_validate {
    template <typename T_model, typename T_Operation>
    static bool checkSpaceDepthOp(OperationValidate<T_model, T_Operation> *opValidate,
                                    const std::string opname, std::string &reason){
        bool result = true;
        auto inputList = ::hal::limitation::nnapi::match("SpaceDepthInput", opValidate->InputArgTypes());
        auto outputList = ::hal::limitation::nnapi::match("SpaceDepthOutput", opValidate->OutputArgTypes());
        if(inputList && outputList){
            bool result = true;
            auto operation = opValidate->OperationForRead();
            auto input = operation.inputs[inputList->ArgPos("input")];
            auto blockSize = operation.inputs[inputList->ArgPos("blockSize")];
            if(opValidate->IsConstantTensor(input)){
                reason += "reject " + opname + " because input is constant\n";
                result = false;
            }
            if(!opValidate->IsInput(blockSize)){
                reason += "reject " + opname + " because blocksize is input\n";
                result = false;
            }
        }else{
            reason += "reject " + opname + " because input data type not support\n";
            result = false;
        }
        return result;
    }
template <typename T_model, typename T_Operation>
class Space2depthValidate : public OperationValidate<T_model, T_Operation> {
   public:
    Space2depthValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return checkSpaceDepthOp(this, "SPACE_TO_DEPTH", reason);
    };
};

template <typename T_model, typename T_Operation>
class Depth2spaceValidate : public OperationValidate<T_model, T_Operation> {
   public:
    Depth2spaceValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return checkSpaceDepthOp(this, "DEPTH_TO_SPACE", reason);
    };
};

template <typename T_model, typename T_Operation>
class Space2BatchValidate : public OperationValidate<T_model, T_Operation> {
   public:
    Space2BatchValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        bool result = true;
        auto inputList = ::hal::limitation::nnapi::match("Space2batchInput", this->InputArgTypes());
        auto outputList = ::hal::limitation::nnapi::match("Space2batchOutput", this->OutputArgTypes());
        if(inputList && outputList){
            auto operation = this->OperationForRead();
            auto input = operation.inputs[inputList->ArgPos("input")];
            auto blockSize = operation.inputs[inputList->ArgPos("blockSize")];
            auto pading = operation.inputs[inputList->ArgPos("padings")];
            if ( this->IsConstantTensor(input)){
                reason += "reject SPACE_TO_BATCH because input is constant\n";
                result = false;
            }
            if (!(this->IsConstantTensor(blockSize) && this->IsConstantTensor(pading))) {
                reason += "reject SPACE_TO_BATCH because blocksize or padding not constant\n";
                result = false;
            }
        }else{
            reason += "reject SPACE_TO_BATCH because input data type not support\n";
            result = false;
        }
        return result;
    };
};

template <typename T_model, typename T_Operation>
class Batch2spaceValidate : public OperationValidate<T_model, T_Operation> {
   public:
    Batch2spaceValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        bool result = true;
        auto inputList = ::hal::limitation::nnapi::match("Batch2spaceInput", this->InputArgTypes());
        auto outputList  = ::hal::limitation::nnapi::match("Batch2spaceOutput", this->OutputArgTypes());
        if(inputList && outputList){
            auto operation = this->OperationForRead();
            auto input = operation.inputs[inputList->ArgPos("input")];
            auto blockSize = operation.inputs[inputList->ArgPos("blockSize")];
            if ( this->IsConstantTensor(input)){
                reason += "reject BATCH_TO_SPACE because input is constant\n";
                result = false;
            }
            if ( !this->IsConstantTensor(blockSize) ) {
                reason += "reject BATCH_TO_SPACE because blocksize or padding not constant\n";
                result = false;
            }
        }else{
            reason += "reject BATCH_TO_SPACE because input data type not support\n";
            result = false;
        }
        return result;
    };
};

}  // end of op_validate
}
}

#endif

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
#ifndef __UNIFIED_PREPARED_MODEL_H__
#define __UNIFIED_PREPARED_MODEL_H__

#include "nnrt/prepared_model.hpp"
namespace nnrt {

class UnifiedPreparedModel : public PreparedModel {
 public:
  UnifiedPreparedModel(Model* model, SharedContextPtr context,
                const std::vector<ExecutionIOPtr> &inputs,
                const std::vector<ExecutionIOPtr> &outputs,
                Interpreter* interpreter = NULL);
  ~UnifiedPreparedModel();

  vsi_nn_graph_t* get() override { return graph_; }

  int prepare() override;

  int execute() override;

  int setInput(uint32_t index, const void* data, size_t length) override;

  int getOutput(uint32_t index, void* data, size_t length) override;

  int updateOutputOperand(uint32_t index, const op::OperandPtr operand_type) override;

  int updateInputOperand(uint32_t index, const op::OperandPtr operand_type) override;

  void setInterpreter(Interpreter* interpreter) override {
      interpreter_ = interpreter;
  }

  std::string signature() override { return model_->signature(); }

 private:
  // Implementation details
  struct Private;
  std::unique_ptr<Private> d;
};
}
#endif
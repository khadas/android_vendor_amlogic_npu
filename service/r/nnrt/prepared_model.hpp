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
#ifndef __OVXLIB_PREPARED_MODEL_H__
#define __OVXLIB_PREPARED_MODEL_H__

#include <vector>

#include "nnrt/model.hpp"
#include "nnrt/interpreter.hpp"
#include "nnrt/shared_context.hpp"
#include "nnrt/default_allocator.hpp"

extern "C" {
struct _vsi_nn_graph;
typedef struct _vsi_nn_graph vsi_nn_graph_t;
}

namespace nnrt
{
class PreparedModel;

using PreparedModelPtr = std::shared_ptr<PreparedModel>;

struct ExecutionIO;

using ExecutionIOPtr = std::shared_ptr<ExecutionIO>;

class PreparedModel
{
    public:
        PreparedModel() {}
        PreparedModel(Model* model, Interpreter* interpreter)
            : model_(model), interpreter_(interpreter) {}
        virtual ~PreparedModel() {};

        virtual vsi_nn_graph_t* get() = 0;
        virtual int prepare() = 0;

        virtual int execute() = 0;

        virtual int setInput(uint32_t index, const void* data, size_t length) = 0;

        virtual int getOutput(uint32_t index, void* data, size_t length) = 0;

        virtual int updateOutputOperand(uint32_t index, const op::OperandPtr operand_type) = 0;

        virtual int updateInputOperand(uint32_t index, const op::OperandPtr operand_type) = 0;

        virtual void setInterpreter(Interpreter* interpreter) = 0;

        virtual std::string signature() = 0;

    protected:
        vsi_nn_graph_t* graph_{nullptr};
        Model* model_{nullptr};
        Interpreter* interpreter_;
        std::shared_ptr<IAllocator> allocator_;
        std::vector<uint8_t*> tensor_handles_;

    private:
        // disable copy constructor
        PreparedModel(const PreparedModel&) = delete;
        PreparedModel(PreparedModel&&) = default;
        PreparedModel& operator=(const PreparedModel&) = delete;

};
}

#endif

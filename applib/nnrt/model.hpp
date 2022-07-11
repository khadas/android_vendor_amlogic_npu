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
#ifndef __OVXLIB_MODEL_H__
#define __OVXLIB_MODEL_H__
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "nnrt/logging.hpp"
#include "nnrt/memory_pool.hpp"
#include "nnrt/types.hpp"

namespace nnrt {
namespace op {
class Operation;
using OperationPtr = std::shared_ptr<Operation>;
class Operand;
using OperandPtr = std::shared_ptr<Operand>;
}  // namespace op

class Model {
   public:
    Model();
    ~Model();

    std::vector<uint32_t>& inputIndexes() { return input_indexes_; }
    const std::vector<uint32_t>& inputIndexes() const { return input_indexes_; }

    const std::vector<uint32_t>& outputIndexes() const { return output_indexes_; }
    const std::vector<uint32_t>& outputIndexes() { return output_indexes_; }

    uint32_t inputIndex(uint32_t index) const;

    uint32_t outputIndex(uint32_t index) const;

    op::OperandPtr operand(uint32_t index) {
        if (operands_.find(index) == operands_.end()) {
            return nullptr;
        }
        return operands_[index];
    }

    op::OperationPtr operation(uint32_t index) {
        if (operations_.find(index) == operations_.end()) {
            return nullptr;
        }
        return operations_[index];
    }

    std::map<uint32_t, op::OperandPtr>& operands() { return operands_; }

    const std::map<uint32_t, op::OperandPtr>& operands() const { return operands_; }

    std::map<uint32_t, op::OperationPtr>& operations() { return operations_; }

    const std::map<uint32_t, op::OperationPtr>& operations() const { return operations_; }

    size_t getOperandSize() { return operands_.size(); }

    size_t getOperationSize() { return operations_.size(); }

    bool isInput(uint32_t index);

    bool isOutput(uint32_t index);

    void identifyInputsAndOutputs(const uint32_t* inputs_ptr,
                                  uint32_t input_count,
                                  const uint32_t* outputs_ptr,
                                  uint32_t output_count);

    op::OperationPtr addOperation(OperationType code,
                                  const uint32_t* inputs,
                                  uint32_t input_size,
                                  const uint32_t* outputs,
                                  uint32_t output_size,
                                  uint32_t* out_index = nullptr);

    op::OperationPtr addOperation(op::OperationPtr new_operation, uint32_t* out_index = nullptr);

    op::OperandPtr cloneOperand(op::OperandPtr operand, int32_t* out_index = nullptr);

    op::OperandPtr addOperand(op::OperandPtr new_operand = nullptr, uint32_t* out_index = nullptr);

    int32_t getOperandIndex(op::OperandPtr operand);

    template <typename T>
    static T* getBuffer(const mem_pool::shared_ref& ref) {
        void* data = nullptr;
        if (ref) {
            data = const_cast<void*>(ref->address_);
            NNRT_LOGI_PRINT("Read from shared reference");
        } else {
            NNRT_LOGE_PRINT("Error while getBuffer");
        }
        return static_cast<T*>(data);
    }

    // template <typename T>
    // T* getModifiableBuffer(op::OperandPtr operand) {
    //    // void* data = nullptr;
    //    // uint32_t index = operand->location.poolIndex;
    //    // uint32_t offset = operand->location.offset;
    //    // size_t length = operand->location.length;
    //    // if (index < pool_.size()) {
    //    //     if (!pool_[index]->modifiable()) {
    //    //         //resetModifiableBuffer(operand);
    //    //         //TODO: Decounter buffer reference.
    //    //         index = pool_.addBuffer(length);
    //    //         operand->location.poolIndex = index;
    //    //         operand->location.offset = 0;
    //    //     }
    //    //     data = pool_[index]->data(offset);
    //    // }
    //    // return static_cast<T*>(data);
    //}

    bool addBuffer(const void* buffer, size_t length, uint32_t index);

    int32_t setOperandValue(op::OperandPtr operand, const void* buffer, size_t length);

    int32_t setOperandValue(uint32_t operand_index, const void* buffer, size_t length);

    int32_t setOperandValueFromMemory(op::OperandPtr operand,
                                      const Memory* memory,
                                      size_t offset,
                                      size_t length);

    int32_t setOperandValueFromMemory(uint32_t operand_index,
                                      const Memory* memory,
                                      size_t offset,
                                      size_t length);

    void relax(bool fast_model);

    void finish() { finalized_ = true; }

    void freezeCompile();

    bool isRelaxed() { return relaxed_; }

    bool isFinished() { return finalized_; }

    bool isCompiled() { return compiled_; }

    bool validate();

    bool isValid() { return valid_; }

    std::vector<op::OperandPtr> getOperands(const std::vector<uint32_t>& indexes);

    std::vector<uint32_t> getConsumers(const op::OperandPtr& operd);

    std::vector<uint32_t> getProducers(const op::OperandPtr& operd);

    void removeOperand(uint32_t index);

    int32_t updateOperand(uint32_t index, const op::OperandPtr operand_type);

    void echo();

    std::string signature() {
        if (!isCompiled()) {
            NNRT_LOGW_PRINT("Uncompiled model doesn't have the signature.");
            return "Not Finished";
        }
        return signature_;
    }

    std::string generateSignature();

    mem_pool::Manager& memory_pool() { return memory_pool_; }

    // TODO: Move mem_refs_ to memory manager
    mem_pool::shared_ref add_memory_reference(const void* address,
                                              size_t len,
                                              bool forceNotAcllocateMem = false) {
        mem_refs_.push_back(memory_pool_.add_reference(address, len, forceNotAcllocateMem));
        return mem_refs_.back();
    }

    // TODO: Move mem_refs_ to memory manager
    mem_pool::shared_ref add_memory_reference(const nnrt::Memory* address,
                                              size_t offset,
                                              size_t len) {
        mem_refs_.push_back(memory_pool_.add_reference(address, offset, len));
        return mem_refs_.back();
    }

    void remove_memory_reference(const mem_pool::shared_ref& ref) {
        if (!ref) return;
        auto it = std::find(mem_refs_.begin(), mem_refs_.end(), ref);
        if (it != mem_refs_.end()) {
            it->reset();
            mem_refs_.erase(it);
        }
    }

    int get_cache_handle() { return cache_handle_; }

    int get_cache_size() { return cache_size_; }

    bool set_cache_handle(int handle){
        if (-1 == handle) return false;
        bool status = false;

#ifdef __linux__
        if (cache_handle_ != -1) {
            NNRT_LOGD_PRINT(
                "Close previous model cache, it's safe because different compilation should have "
                "some model structure\n");
            close(cache_handle_);
            cache_handle_ = -1;
        }

        auto is_writeable = [handle]() {
            int test_data = 0x5A5A5A5A;
            size_t length = write(handle, &test_data, sizeof(test_data));
            lseek(handle, 0, SEEK_SET);
            ftruncate(handle, 0);  // reset file content

            return (length == sizeof(test_data));
        };

        cache_size_ = lseek(handle, 0, SEEK_END);
        lseek(handle, 0, SEEK_SET);

        if (cache_size_ || is_writeable()) {
            cache_handle_ = dup(handle);
            status = (cache_handle_ != -1);
        } else {
            NNRT_LOGD_PRINT("Set cache handle failed");
            cache_handle_ = -1;
            cache_size_ = 0;
        }
#endif
        return status;
    }

    bool allocate_cache_memory() {
#ifdef __linux__
        auto flag = (-1 == cache_handle_) ? (MAP_SHARED | MAP_ANONYMOUS) : (MAP_SHARED);
        cache_memory_ = mmap(nullptr, cache_size_, PROT_READ, flag, cache_handle_, 0);
        return (cache_memory_ == nullptr) ? false : true;
#endif
        return false;
    }
    const char* get_cache_memory() { return (const char*)cache_memory_; }

    bool replace_model_with_nbg() {
        if (cache_handle_ == -1 || cache_size_ == 0) return true;
        operations_.clear();
        operation_unique_id_ = 0;
        const uint32_t* inputs = input_indexes_.data();
        uint32_t input_size = input_indexes_.size();
        const uint32_t* outputs = output_indexes_.data();
        uint32_t output_size = output_indexes_.size();
        uint32_t* out_index = nullptr;
        addOperation(OperationType::NBG, inputs, input_size, outputs, output_size, out_index);
        return allocate_cache_memory();
    }

   private:
    uint32_t operand_unique_id_{0};
    uint32_t operation_unique_id_{0};
    bool relaxed_{false}; /* the flag to run fp16 data,instead of fp32*/
    bool finalized_{false};
    bool compiled_{false};
    bool valid_{false};

    int cache_handle_{-1};
    int cache_size_{0};
    void* cache_memory_{nullptr};

    std::string signature_;
    std::map<uint32_t, op::OperationPtr> operations_;
    std::map<uint32_t, op::OperandPtr> operands_;
    std::vector<uint32_t> input_indexes_;
    std::vector<uint32_t> output_indexes_;

    mem_pool::Manager memory_pool_;
    std::list<mem_pool::shared_ref> mem_refs_;
};

using ModelPtr = std::shared_ptr<Model>;
}  // namespace nnrt

#endif

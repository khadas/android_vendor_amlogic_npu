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
#ifndef __NNRT_DEFAULT_ALLOCATOR_H__
#define __NNRT_DEFAULT_ALLOCATOR_H__

#include <cstdlib>
#include <map>
#include "allocator.hpp"
namespace nnrt {

class DefaultAlloctor : public IAllocator {
 public:
  DefaultAlloctor() = default;
  ~DefaultAlloctor() = default;

  uint8_t* Allocate(uint32_t size) override {
    uint8_t* origin_buffer = (uint8_t*)malloc(GetRealSize(size));
    uint8_t* aliged_buffer = MemoryAlign(origin_buffer);
    uint64_t* origin_address = (uint64_t*)(aliged_buffer - sizeof(void*));
    *origin_address = (uint64_t)origin_buffer;

    return aliged_buffer;
  }

  void Release(uint8_t* buffer) override {
    uint64_t* origin_address = (uint64_t*)(buffer - sizeof(void*));
    uint8_t* origin_buffer = (uint8_t*)(*origin_address);
    free(origin_buffer);
  }
};
}
#endif
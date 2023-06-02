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
#ifndef __NNRT_ALLOCATOR_H__
#define __NNRT_ALLOCATOR_H__

#include <cstdint>
namespace nnrt {

static const int align_size = 64;
static const int guard_size = 64;

class IAllocator {
 public:
  IAllocator() = default;
  virtual ~IAllocator() = default;

  virtual uint8_t* Allocate(uint32_t size) = 0;
  virtual void Release(uint8_t* buffer) = 0;

 protected:
  inline uint32_t GetRealSize(uint32_t size) {
    return size + 2 * guard_size + (align_size - 1);
  }

  inline uint8_t* MemoryAlign(uint8_t* buffer) {
    uint8_t* aligned_buffer;
    buffer += guard_size;
    aligned_buffer =
        (uint8_t*)(((uint64_t)buffer + (align_size - 1)) & ~(align_size - 1));

    return aligned_buffer;
  }
};
}
#endif
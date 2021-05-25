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
#ifndef ANDROID_NN_VSI_DRIVER_SANDBOX_H
#define ANDROID_NN_VSI_DRIVER_SANDBOX_H
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace android {
namespace nn {
namespace vsi_driver {

class SandBox {
   public:
    using ProcFunc = std::function<bool()>;

    SandBox();

    ~SandBox();

    void push(ProcFunc proc);
    void wait(ProcFunc proc);

   protected:
    ProcFunc pull();
    void loop();

   private:
    std::thread thread_;

    std::mutex proc_que_mux_;
    std::deque<ProcFunc> proc_que_;
    std::condition_variable cv_;

    bool wait_;
    std::mutex wait_mtx_;
    std::condition_variable wait_cv_;
};
}  // namespace vsi_driver
}  // namespace nn
}  // namespace android
#endif

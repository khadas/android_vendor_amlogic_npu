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
#include <android-base/logging.h>
#include <iostream>
#include "SandBox.h"
#define LOG_TAG "SANDBOX"

namespace android {
namespace nn {
namespace vsi_driver {

SandBox::SandBox() {
    thread_ = std::thread(std::bind(&SandBox::loop, this));
}

SandBox::~SandBox() {
    thread_.join();
}

void SandBox::push(SandBox::ProcFunc proc) {
    {
        std::lock_guard<std::mutex> lock(proc_que_mux_);
        proc_que_.push_back(proc);
    }
    cv_.notify_one();
    // submit singal
}

void SandBox::wait(SandBox::ProcFunc proc) {
    static std::mutex single_wait;
    std::lock_guard<std::mutex> g(single_wait);
    {
        std::lock_guard<std::mutex> guard(wait_mtx_);
        wait_ = true;
    }

    push(proc);

    std::unique_lock<std::mutex> lock(wait_mtx_);
    wait_cv_.wait(lock, [this] { return !this->wait_; });
}

SandBox::ProcFunc SandBox::pull() {
    {
        std::unique_lock<std::mutex> lock(proc_que_mux_);
        cv_.wait(lock, [this] { return !(this->proc_que_.empty()); });
        auto proc = proc_que_.front();

        proc_que_.pop_front();
        return proc;
    }
}

void SandBox::loop() {
    while (1) {
        auto proc = pull();
        bool exit = proc();
        LOG(INFO) << __FILE__ << ":"<< __FUNCTION__ <<"->proc done";

        {
            std::lock_guard<std::mutex> guard(wait_mtx_);
            wait_ = false;
        }
        wait_cv_.notify_one();

        if (exit) {
            break;
        }
    }
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

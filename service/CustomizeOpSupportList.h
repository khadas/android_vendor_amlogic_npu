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
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "VsiPlatform.h"

namespace android {
namespace nn {
namespace vsi_driver {

// blacklist content:
// single line include operation index defined by android nn spec
// each number should end with ","
static inline bool IsOpBlockedByBlockList(int32_t op_type) {
    char env[128] = {0};
    (void)getSystemProperty("NN_DBG_OP_BLK_LIST", env);
    if (strlen(env) == 0) return false;

    std::fstream fs(env);
    if (!fs.good()) {
        LOG(INFO) << "can not open blocklist file from -> " << env;
        // Ignore if no blocklist found
        return false;
    }
    std::string op_list_line;  // = fs.getline();
    std::getline(fs, op_list_line);
    std::vector<int32_t> op_list;

    const char* splitor = ",";

    auto end = op_list_line.find(splitor);
    decltype(end) start = -1;
    while (end != op_list_line.npos && end != start) {
        auto value = op_list_line.substr(start + 1, end - start - 1);
        start = op_list_line.find(splitor, end);
        end = op_list_line.find(splitor, start + 1);
        op_list.push_back(std::stoi(value));
    }

    return op_list.end() !=
           std::find(op_list.begin(), op_list.end(), static_cast<int32_t>(op_type));
}

static inline bool IsOpBlocked(int32_t op_type) {
    // put operand id to block_list
    // https://developer.android.com/ndk/reference/group/neural-networks
    // to reject ANEURALNETWORKS_ADD,
    // block_list = {0};
    std::vector<int32_t> customer_specific_block_list = {};

    bool blocked = IsOpBlockedByBlockList(op_type) ||
           std::find(customer_specific_block_list.cbegin(),
                     customer_specific_block_list.cend(),
                     op_type) != customer_specific_block_list.cend();
    if (blocked) {
        LOG(INFO) << "Op_type(" << op_type << ") blocked by customer or block list";
    }

    return blocked;
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

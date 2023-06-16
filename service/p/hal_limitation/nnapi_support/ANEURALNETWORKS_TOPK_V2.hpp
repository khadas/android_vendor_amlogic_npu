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

#ifndef __ANEURALNETWORKS_TOPK_V2_HPP__
#define __ANEURALNETWORKS_TOPK_V2_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME TopkV2Input
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,     \
    top_k)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(topk_v2)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .top_k_(nnrt::OperandType::INT32));

    OVERRIDE_SPEC(topk_v2, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16));

    // OVERRIDE_SPEC(topk_v2, int32)
    // .input_(nnrt::OperandType::TENSOR_INT32));

    // OVERRIDE_SPEC(topk_v2, asymm_u8)
    // .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME TopkV2Output
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
    output0,              \
    output1)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(output)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .output0_(nnrt::OperandType::TENSOR_FLOAT32)
    .output1_(nnrt::OperandType::TENSOR_INT32));

    OVERRIDE_SPEC(output, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .output0_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(output, int32)
    .input_(nnrt::OperandType::TENSOR_INT32)
    .output0_(nnrt::OperandType::TENSOR_INT32));

    OVERRIDE_SPEC(output, asymm_u8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .output0_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif
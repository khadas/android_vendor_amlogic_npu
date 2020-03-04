/****************************************************************************
*
*    Copyright (c) 2019 Vivante Corporation
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

/**
     * Computes the softmax activation on the input tensor element-wise, per
     * batch, by normalizing the input vector so the maximum coefficient is
     * zero.
     *
     * The output is calculated using this formula:
     *
     *     output[batch, i] =
     *         exp((input[batch, i] - max(input[batch, :])) * beta) /
     *         sum_{k}{exp((input[batch, k] - max(input[batch, :])) * beta)}
     *
     * For input tensor with rank other than 2, the activation will be applied
     * independently on each 1-D slice along specified dimension.
     *
     * Supported tensor {@link OperandCode}:
     * * {@link ANEURALNETWORKS_TENSOR_FLOAT16} (since API level 29)
     * * {@link ANEURALNETWORKS_TENSOR_FLOAT32}
     * * {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM}
     *
     * Supported tensor rank: up to 4.
     * Tensors with rank other than 2 or 4 are only supported since API level 29.
     *
     * Inputs:
     * * 0: A 2-D or 4-D tensor, specifying the tensor to be reshaped. Since
     *      API level 29, this tensor may be zero-sized.
     * * 1: A scalar, specifying the positive scaling factor for the exponent,
     *      beta. If input0 is of {@link ANEURALNETWORKS_TENSOR_FLOAT32} or
     *      {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM}, the scalar must be of
     *      {@link ANEURALNETWORKS_FLOAT32}. If input0 is of {@link
     *      ANEURALNETWORKS_TENSOR_FLOAT16}, then the scalar must be of {@link
     *      ANEURALNETWORKS_FLOAT16}.
     * * 2: An optional {@link ANEURALNETWORKS_INT32} scalar, default to -1,
     *      specifying the dimension the activation would be performed on.
     *      Negative index is used to specify axis from the end (e.g. -1 for
     *      the last axis). Must be in the range [-n, n).
     *      Available since API level 29.
     *
     * Outputs:
     * * 0: The output tensor of same shape as input0.
     *      For {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM},
     *      the scale must be 1.f / 256 and the zeroPoint must be 0.
     *
     * Available since API level 27.
     */

#ifndef __ANEURALNETWORKS_SOFTMAX_HPP__
#define __ANEURALNETWORKS_SOFTMAX_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME SoftmaxInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,     \
    scaling,    \
    axis)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(softmax)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .scaling_(nnrt::OperandType::TENSOR_FLOAT32)
    .axis_(nnrt::OperandType::INT32, OPTIONAL));

    // Note: not support scaler float16
    // OVERRIDE_SPEC(softmax, float16)
    // .input_(nnrt::OperandType::TENSOR_FLOAT16)
    // .scaling_(nnrt::OperandType::FLOAT16));

    OVERRIDE_SPEC(softmax, asysm_u8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME SoftmaxOutput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
    output)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(output)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_(nnrt::OperandType::TENSOR_FLOAT32));

    OVERRIDE_SPEC(output, 0)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .output_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(output, 1)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .output_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif
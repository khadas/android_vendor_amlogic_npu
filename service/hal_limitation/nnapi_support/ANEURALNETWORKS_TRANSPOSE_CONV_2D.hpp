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
 * Performs the transpose of 2-D convolution operation.
 *
 * This operation is sometimes called "deconvolution" after Deconvolutional
 * Networks, but is actually the transpose (gradient) of
 * {@link ANEURALNETWORKS_CONV_2D} rather than an actual deconvolution.
 *
 * The output dimensions are functions of the filter dimensions, stride, and
 * padding.
 *
 * Supported tensor {@link OperandCode} configurations:
 * * 16 bit floating point:
 * * * {@link ANEURALNETWORKS_TENSOR_FLOAT16} for input, filter, output, and bias.
 *
 * * 32 bit floating point:
 * * * {@link ANEURALNETWORKS_TENSOR_FLOAT32} for input, filter, output, and bias.
 *
 * * Quantized:
 * * * {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM} for input, filter, and output.
 * * * {@link ANEURALNETWORKS_TENSOR_INT32} for bias (with scale set to
 * * * input.scale * filter.scale).
 *
 * * Quantized with symmetric per channel quantization for the filter:
 * * * {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM} for input, and output.
 * * * {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL} for filter.
 * * * {@link ANEURALNETWORKS_TENSOR_INT32} for bias (scale set to 0.0,
 * * * each value scaling is separate and equal to input.scale * filter.scales[channel]).
 *
 * Supported tensor rank: 4, with "NHWC" or "NCHW" data layout.
 * With the default data layout NHWC, the data is stored in the order of:
 * [batch, height, width, channels]. Alternatively, the data layout could
 * be NCHW, the data storage order of: [batch, channels, height, width].
 *
 * Both explicit padding and implicit padding are supported.
 *
 * Inputs (explicit padding):
 * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
 *      specifying the input.
 * * 1: A 4-D tensor, of shape
 *      [depth_out, filter_height, filter_width, depth_in], specifying the
 *      filter. For tensor of type
 *      {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL} the channel
 *      dimension (extraParams.channelQuant.channelDim) must be set to 0.
 * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
 *      tensor of type {@link ANEURALNETWORKS_TENSOR_FLOAT32} or
 *      {@link ANEURALNETWORKS_TENSOR_FLOAT16}, the bias should be of the
 *      same type. For input tensor of type
 *      {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM}, the bias should be
 *      of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint of 0 and
 *      bias_scale == input_scale * filter_scale. For filter tensor of
 *      {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL}, the bias
 *      must be of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint of
 *      0 and bias_scale of 0. The actual scale of each value 'i' is equal
 *      to bias_scale[i] = input_scale * filter_scale[i].
 * * 3: An {@link ANEURALNETWORKS_INT32} scalar, specifying the padding on
 *      the left, in the ‘width’ dimension.
 * * 4: An {@link ANEURALNETWORKS_INT32} scalar, specifying the padding on
 *      the right, in the ‘width’ dimension.
 * * 5: An {@link ANEURALNETWORKS_INT32} scalar, specifying the padding on
 *      the top, in the ‘height’ dimension.
 * * 6: An {@link ANEURALNETWORKS_INT32} scalar, specifying the padding on
 *      the bottom, in the ‘height’ dimension.
 * * 7: An {@link ANEURALNETWORKS_INT32} scalar, specifying the stride when
 *      walking through input in the ‘width’ dimension.
 * * 8: An {@link ANEURALNETWORKS_INT32} scalar, specifying the stride when
 *      walking through input in the ‘height’ dimension.
 * * 9: An {@link ANEURALNETWORKS_INT32} scalar, and has to be one of the
 *      {@link FuseCode} values. Specifies the activation to
 *      invoke on the result.
 * * 10: An {@link ANEURALNETWORKS_BOOL} scalar, set to true to specify
 *       NCHW data layout for input0 and output0. Set to false for NHWC.
 *
 * Inputs (implicit padding):
 * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
 *      specifying the input.
 * * 1: A 4-D tensor, of shape
 *      [depth_out, filter_height, filter_width, depth_in], specifying the
 *      filter. For tensor of type
 *      {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL} the channel
 *      dimension (extraParams.channelQuant.channelDim) must be set to 0.
 * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
 *      tensor of type {@link ANEURALNETWORKS_TENSOR_FLOAT32} or
 *      {@link ANEURALNETWORKS_TENSOR_FLOAT16}, the bias should be of the
 *      same type. For input tensor of type
 *      {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM}, the bias should be
 *      of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint of 0 and
 *      bias_scale == input_scale * filter_scale. For filter tensor of
 *      {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL}, the bias
 *      must be of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint of
 *      0 and bias_scale of 0. The actual scale of each value 'i' is equal
 *      to bias_scale[i] = input_scale * filter_scale[i].
 * * 3: An {@link ANEURALNETWORKS_TENSOR_INT32} tensor, specifying the output
 *      tensor shape.
 * * 4: An {@link ANEURALNETWORKS_INT32} scalar, specifying the implicit
 *      padding scheme, has to be one of the
 *      {@link PaddingCode} values.
 * * 5: An {@link ANEURALNETWORKS_INT32} scalar, specifying the stride when
 *      walking through input in the ‘width’ dimension.
 * * 6: An {@link ANEURALNETWORKS_INT32} scalar, specifying the stride when
 *      walking through input in the ‘height’ dimension.
 * * 7: An {@link ANEURALNETWORKS_INT32} scalar, and has to be one of the
 *      {@link FuseCode} values. Specifies the activation to
 *      invoke on the result.
 * * 8: An {@link ANEURALNETWORKS_BOOL} scalar, set to true to specify
 *      NCHW data layout for input0 and output0. Set to false for NHWC.
 *
 * Outputs:
 * * 0: The output 4-D tensor, of shape
 *      [batches, out_height, out_width, depth_out].
 *
 * Available since API level 29.
 */

#ifndef __ANEURALNETWORKS_TRANSPOSE_CONV_2D_HPP__
#define __ANEURALNETWORKS_TRANSPOSE_CONV_2D_HPP__

#include "hal_limitation/support_macros.hpp"

#define OP_SPEC_NAME TransposeConv2DOperationInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,                 \
     kernel,           \
     bias,           \
     pad_left,  \
     pad_right, \
     pad_top,   \
     pad_bottom,    \
     stride_w,  \
     stride_h, \
     fuse_code, \
     layout,    \
     output_shape,  \
     implicit_pad)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(explicit_padding_transpose_conv_2d)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT32)
    .bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .pad_left_(nnrt::OperandType::INT32)
    .pad_top_(nnrt::OperandType::INT32)
    .pad_right_(nnrt::OperandType::INT32)
    .pad_bottom_(nnrt::OperandType::INT32)
    .stride_w_(nnrt::OperandType::INT32)
    .stride_h_(nnrt::OperandType::INT32)
    .fuse_code_(nnrt::OperandType::INT32)
    .layout_(nnrt::OperandType::BOOL));

    // // Note: Bia not support float16
    // OVERRIDE_SPEC(explicit_padding_transpose_conv_2d, float16)
    // .input_(nnrt::OperandType::TENSOR_FLOAT16)
    // .kernel_(nnrt::OperandType::TENSOR_FLOAT16)
    // .bias_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(explicit_padding_transpose_conv_2d, quant8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .kernel_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bias_(nnrt::OperandType::TENSOR_INT32));

    // // Note: Kernel not support perchannel
    // OVERRIDE_SPEC(explicit_padding_transpose_conv_2d, per_channel_quant8)
    // .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .kernel_(nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
    // .bias_(nnrt::OperandType::TENSOR_INT32));

MAKE_SPEC(implicit_padding_transpose_conv_2d)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT32)
    .bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_shape_(nnrt::OperandType::TENSOR_INT32)
    .implicit_pad_(nnrt::OperandType::INT32)
    .stride_w_(nnrt::OperandType::INT32)
    .stride_h_(nnrt::OperandType::INT32)
    .fuse_code_(nnrt::OperandType::INT32)
    .layout_(nnrt::OperandType::BOOL));

    // OVERRIDE_SPEC(implicit_padding_transpose_conv_2d, float16)
    // .input_(nnrt::OperandType::TENSOR_FLOAT16)
    // .kernel_(nnrt::OperandType::TENSOR_FLOAT16)
    // .bias_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(implicit_padding_transpose_conv_2d, quant8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .kernel_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bias_(nnrt::OperandType::TENSOR_INT32));

    // OVERRIDE_SPEC(implicit_padding_transpose_conv_2d, per_channel_quant8)
    // .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .kernel_(nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
    // .bias_(nnrt::OperandType::TENSOR_INT32));
#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

// Output Spec
#define OP_SPEC_NAME TransposeConv2DOperationOutput
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

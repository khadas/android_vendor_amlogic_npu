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
     * Performs a depthwise 2-D convolution operation.
     *
     * Given an input tensor of shape [batches, height, width, depth_in] and a
     * filter tensor of shape [1, filter_height, filter_width, depth_out]
     * containing depth_out convolutional filters of depth 1, DEPTHWISE_CONV
     * applies a different filter to each input channel (expanding from 1
     * channel to channel_multiplier channels for each), then concatenates the
     * results together.
     *
     * The output has depth_out = depth_in * depth_multiplier channels.
     * The output dimensions are functions of the filter dimensions, stride, and
     * padding.
     *
     * The values in the output tensor are computed as:
     *
     *     output[b, i, j, k * channel_multiplier + q] =
     *         sum_{di, dj} (
     *             input[b, strides[1] * i + di, strides[2] * j + dj, k] *
     *             filter[1, di, dj, k * channel_multiplier + q]
     *         ) + bias[k * channel_multiplier + q]
     *
     * Supported tensor {@link OperandCode} configurations:
     * * 32 bit floating point:
     * * * {@link ANEURALNETWORKS_TENSOR_FLOAT32} for input, filter, output, and bias.
     *
     * * Quantized:
     * * * {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM} for input, filter, and output.
     * * * {@link ANEURALNETWORKS_TENSOR_INT32} for bias (with scale set to
     * * * input.scale * filter.scale).
     *
     * Available since API level 29:
     * * 16 bit floating point:
     * * * {@link ANEURALNETWORKS_TENSOR_FLOAT16} for input, filter, output, and bias.
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
     * * 1: A 4-D tensor, of shape [1, filter_height, filter_width, depth_out],
     *      specifying the filter. For tensor of type
     *      {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL} the channel
     *      dimension (extraParams.channelQuant.channelDim) must be set to 3.
     * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
     *      tensor of type {@link ANEURALNETWORKS_TENSOR_FLOAT32} or
     *      {@link ANEURALNETWORKS_TENSOR_FLOAT16}, the bias must be of the same
     *      type. For filter tensor of {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM},
     *      the bias should be of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint
     *      of 0 and bias_scale == input_scale * filter_scale. For filter tensor
     *      of {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL}, the bias
     *      should be of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint of
     *      0 and bias_scale of 0. The actual scale of each value 'i' is equal to
     *      bias_scale[i] = input_scale * filter_scale[i].
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
     * * 9: An {@link ANEURALNETWORKS_INT32} scalar, specifying the depthwise
     *      multiplier.
     * * 10: An {@link ANEURALNETWORKS_INT32} scalar, and has to be one of the
     *       {@link FuseCode} values. Specifies the activation to
     *       invoke on the result.
     * * 11: An optional {@link ANEURALNETWORKS_BOOL} scalar, default to false.
     *       Set to true to specify NCHW data layout for input0 and output0.
     *       Available since API level 29.
     * * 12: An optional {@link ANEURALNETWORKS_INT32} scalar, specifying the dilation
     *      factor for width. Defaults to 1. If set to k > 1, there will be k-1 skipped
     *      cells between each filter element on width dimension. If this input is set,
     *      input 13 (dilation factor for height) must be specified as well.
     *      Available since API level 29.
     * * 13: An optional {@link ANEURALNETWORKS_INT32} scalar, specifying the dilation
     *      factor for height. Defaults to 1. If set to k > 1, there will be k-1 skipped
     *      cells between each filter element on height dimension. If this input is set,
     *      input 12 (dilation factor for width) must be specified as well.
     *      Available since API level 29.
     *
     * Inputs (implicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
     *      specifying the input.
     * * 1: A 4-D tensor, of shape [1, filter_height, filter_width, depth_out],
     *      specifying the filter.
     * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
     *      tensor of type {@link ANEURALNETWORKS_TENSOR_FLOAT32} or
     *      {@link ANEURALNETWORKS_TENSOR_FLOAT16}, the bias must be of the same
     *      type. For filter tensor of {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM},
     *      the bias should be of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint
     *      of 0 and bias_scale == input_scale * filter_scale. For filter tensor
     *      of {@link ANEURALNETWORKS_TENSOR_QUANT8_SYMM_PER_CHANNEL}, the bias
     *      should be of {@link ANEURALNETWORKS_TENSOR_INT32}, with zeroPoint of
     *      0 and bias_scale of 0. The actual scale of each value 'i' is equal to
     *      bias_scale[i] = input_scale * filter_scale[i].
     * * 3: An {@link ANEURALNETWORKS_INT32} scalar, specifying the implicit
     *      padding scheme, has to be one of the
     *      {@link PaddingCode} values.
     * * 4: An {@link ANEURALNETWORKS_INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 5: An {@link ANEURALNETWORKS_INT32} scalar, specifying the stride when
     *      walking through input in the ‘height’ dimension.
     * * 6: An {@link ANEURALNETWORKS_INT32} scalar, specifying the depthwise
     *      multiplier.
     * * 7: An {@link ANEURALNETWORKS_INT32} scalar, and has to be one of the
     *      {@link FuseCode} values. Specifies the activation to
     *      invoke on the result.
     * * 8: An optional {@link ANEURALNETWORKS_BOOL} scalar, default to false.
     *      Set to true to specify NCHW data layout for input0 and output0.
     *      Available since API level 29.
     * * 9: An optional {@link ANEURALNETWORKS_INT32} scalar, specifying the dilation
     *      factor for width. Defaults to 1. If set to k > 1, there will be k-1 skipped
     *      cells between each filter element on width dimension. If this input is set,
     *      input 10 (dilation factor for height) must be specified as well.
     *      Available since API level 29.
     * * 10: An optional {@link ANEURALNETWORKS_INT32} scalar, specifying the dilation
     *      factor for height. Defaults to 1. If set to k > 1, there will be k-1 skipped
     *      cells between each filter element on height dimension. If this input is set,
     *      input 9 (dilation factor for width) must be specified as well.
     *      Available since API level 29.
     *
     * Outputs:
     * * 0: The output 4-D tensor, of shape
     *      [batches, out_height, out_width, depth_out]. Before API level 29,
     *      for output tensor of {@link ANEURALNETWORKS_TENSOR_QUANT8_ASYMM},
     *      the following condition must be satisfied:
     *      output_scale > input_scale * filter_scale
     *
     * Available since API level 27.
     */

#ifndef __ANEURALNETWORKS_DEPTHWISE_CONV_2D_HPP__
#define __ANEURALNETWORKS_DEPTHWISE_CONV_2D_HPP__

#include "hal_limitation/support_macros.hpp"

#define OP_SPEC_NAME DepthwiseConvolution2DInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
     kernel,              \
     bias,                \
     explicit_pad_left,   \
     explicit_pad_right,  \
     explicit_pad_top,    \
     explicit_pad_bottom, \
     stride_w,            \
     stride_h,            \
     fuse_code,           \
     data_layout,         \
     dilation_w,          \
     dilation_h,          \
     multiplier,          \
     implicit_pad_type)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(explict_padding_base)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT32)
    .bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .explicit_pad_left_(nnrt::OperandType::INT32)
    .explicit_pad_right_(nnrt::OperandType::INT32)
    .explicit_pad_top_(nnrt::OperandType::INT32)
    .explicit_pad_bottom_(nnrt::OperandType::INT32)
    .stride_w_(nnrt::OperandType::INT32)
    .stride_h_(nnrt::OperandType::INT32)
    .multiplier_(nnrt::OperandType::INT32)
    .fuse_code_(nnrt::OperandType::INT32)
    .data_layout_(nnrt::OperandType::BOOL, OPTIONAL)
    .dilation_w_(nnrt::OperandType::INT32, OPTIONAL)
    .dilation_h_(nnrt::OperandType::INT32, OPTIONAL));

// // Note: Bias not support float16
// OVERRIDE_SPEC(explict_padding_base, in_float16)
//     .input_(nnrt::OperandType::TENSOR_FLOAT16)
//     .kernel_(nnrt::OperandType::TENSOR_FLOAT16)
//     .bias_(nnrt::OperandType::TENSOR_FLOAT16));

OVERRIDE_SPEC(explict_padding_base, in_asysm_u8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .kernel_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bias_(nnrt::OperandType::TENSOR_INT32));

// Note: Kernel not support perchannel now
// OVERRIDE_SPEC(explict_padding_base, perchan_quant)
//     .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
//     .kernel_(nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
//     .bias_(nnrt::OperandType::TENSOR_INT32));

MAKE_SPEC(implicit_padding_base)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT32)
    .bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .implicit_pad_type_(nnrt::OperandType::INT32)
    .stride_w_(nnrt::OperandType::INT32)
    .stride_h_(nnrt::OperandType::INT32)
    .multiplier_(nnrt::OperandType::INT32)
    .fuse_code_(nnrt::OperandType::INT32)
    .data_layout_(nnrt::OperandType::BOOL, OPTIONAL)
    .dilation_w_(nnrt::OperandType::INT32, OPTIONAL)
    .dilation_h_(nnrt::OperandType::INT32, OPTIONAL));

OVERRIDE_SPEC(implicit_padding_base, in_float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT16)
    .bias_(nnrt::OperandType::TENSOR_FLOAT16));

OVERRIDE_SPEC(implicit_padding_base, in_asysm_u8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .kernel_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bias_(nnrt::OperandType::TENSOR_INT32));

// OVERRIDE_SPEC(implicit_padding_base, perchan_quant)
//     .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
//     .kernel_(nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
//     .bias_(nnrt::OperandType::TENSOR_INT32));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

// Output Spec
#define OP_SPEC_NAME DepthwiseConvolution2DOutput
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
/****************************************************************************
*
*    Copyright (c) 2018 Vivante Corporation
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
#pragma once
#ifndef _OPENVX_EXT_LIBNNEXT_H_
#define _OPENVX_EXT_LIBNNEXT_H_
#include <VX/vx.h>
#include <VX/vx_types.h>

#define gcoMATH_MIN(X, Y) (((X) < (Y))?(X):(Y))
#define gcoMATH_MAX(X, Y) (((X) > (Y))?(X):(Y))
#define DIM_SIZE 4

#ifdef __cplusplus
extern "C" {
#endif

/*! if there are more than 1 kernel in solution
the KERNEL_ENUM_LIBNNEXT_OFFSET must be modified keep different for any kernel
*/
enum vx_kernel_libnnext_offset_e
{
    KERNEL_ENUM_LIBNNEXT_OFFSET = 1,
    KERNEL_ENUM_PREMUTE_OFFSET,
    KERNEL_ENUM_PRIORBOX_OFFSET = 2 + KERNEL_ENUM_PREMUTE_OFFSET,
    KERNEL_ENUM_FLATTEN_OFFSET,
    KERNEL_ENUM_L2NORMALIZESCALE_OFFSET,
    KERNEL_ENUM_PARAMETRICRELU_OFFSET,
    KERNEL_ENUM_PREBBOX_OFFSET = 3 + KERNEL_ENUM_PARAMETRICRELU_OFFSET,
    KERNEL_ENUM_ADD_RELU_KERNEL_OFFSET,
    KERNEL_ENUM_POOLING_WITH_ARGMAX_OFFSET,
    KERNEL_ENUM_UNPOOLING_OFFSET = 2 + KERNEL_ENUM_POOLING_WITH_ARGMAX_OFFSET,
    KERNEL_ENUM_ARGMAX_OFFSET = 2 + KERNEL_ENUM_UNPOOLING_OFFSET,
    KERNEL_ENUM_ALEXNET_GEMM_OFFSET = 2 + KERNEL_ENUM_ARGMAX_OFFSET,
    KERNEL_ENUM_IMG2COL_DILATED_OFFSET,
    KERNEL_ENUM_IMG2COL_DILATED_INT8_OFFSET,
    KERNEL_ENUM_ALEXNET_GEMM_INT8_OFFSET,
    KERNEL_ENUM_ELTWISE_MAX,
    KERNEL_ENUM_FULLYCONNECTED_AXIS2,
    KERNEL_ENUM_TENSORCROP_INT16,
    KERNEL_ENUM_TENSORCROP_INT8,
    KERNEL_ENUM_TENSORCROP_INT16_FP16,
    KERNEL_ENUM_DROPOUT,
    KERNEL_ENUM_SHUFFLECHANNEL,
    KERNEL_ENUM_RESIZE,
    KERNEL_ENUM_REVERSE,
    KERNEL_ENUM_RESIZE_16BITS_DOWNSAMPLE_QUARTER,
    KERNEL_ENUM_RESIZE_8BITS_DOWNSAMPLE_QUARTER,
    KERNEL_ENUM_SCALE,
    KERNEL_ENUM_TENSORREVERSE,
    KERNEL_ENUM_TENSORELU_OFFSET,
    KERNEL_ENUM_SPACE2BATCH,
    KERNEL_ENUM_BATCH2SPACE,
    KERNEL_ENUM_SPACE2DEPTH,
    KERNEL_ENUM_IMAGEPROCESS,
    KERNEL_ENUM_SCALETOTENSOR,
    KERNEL_ENUM_GEMM,
    KERNEL_ENUM_LAYERNORM,
    KERNEL_ENUM_LAYERNORMFP16TOU8_OFFSET,
    KERNEL_ENUM_REDUCE,
    KERNEL_ENUM_INSTANCENORM,
    KERNEL_ENUM_TENSORSTACKCONCAT,
    KERNEL_ENUM_TENSORSTACKCONCAT8BITS_OFFSET,
    KERNEL_ENUM_SIGNALFRAME,
    KERNEL_ENUM_RELATIONALOPS,
    KERNEL_ENUM_SYNC_HOST,
    KERNEL_ENUM_POW,
    KERNEL_ENUM_FLOORDIV,
    KERNEL_ENUM_MINIMUM,
    KERNEL_ENUM_SPATIAL_TRANSFORMER,
    KERNEL_ENUM_LOGICAL_OPS,
    KERNEL_ENUM_SELECT,
    KERNEL_ENUM_LSTMUNIT_ACTIVATION,
    KERNEL_ENUM_TENSOR_ADD_MEAN_STDDEV_NORM,
    KERNEL_ENUM_STACK,
};

//! [KERNEL NAME]
#define VX_KERNEL_NAME_PERMUTECWH                          "com.vivantecorp.extension.vxcPermuteCWH"
#define VX_KERNEL_NAME_PERMUTECHW                          "com.vivantecorp.extension.vxcPermuteCWH"
#define VX_KERNEL_NAME_PRIORBOX                            "com.vivantecorp.extension.vxcPriorBox"
#define VX_KERNEL_NAME_FLATTEN                             "com.vivantecorp.extension.flatten"
#define VX_KERNEL_NAME_L2NORMALIZESCALE                    "com.vivantecorp.extension.vxcL2NormalizeScale"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT                "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT_INT8           "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt_int8"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT_UINT8          "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt_uint8"
#define VX_KERNEL_NAME_L2NORMSCALE_SUMRSQRT_INT16          "com.vivantecorp.extension.vxcL2NormScale_SumRsqrt_int16"
#define VX_KERNEL_NAME_L2NORMSCALE_FP16TOFP16              "com.vivantecorp.extension.vxcL2NormScale_Fp16toFp16"
#define VX_KERNEL_NAME_L2NORMSCALE_INT8TOINT8              "com.vivantecorp.extension.vxcL2NormScale_Int8toInt8"
#define VX_KERNEL_NAME_L2NORMSCALE_INT8TOFP16              "com.vivantecorp.extension.vxcL2NormScale_Int8toFp16"
#define VX_KERNEL_NAME_L2NORMSCALE_UINT8TOUINT8            "com.vivantecorp.extension.vxcL2NormScale_UInt8toUInt8"
#define VX_KERNEL_NAME_L2NORMSCALE_UINT8TOFP16             "com.vivantecorp.extension.vxcL2NormScale_UInt8toFp16"
#define VX_KERNEL_NAME_L2NORMSCALE_INT16TOINT16            "com.vivantecorp.extension.vxcL2NormScale_Int16toInt16"
#define VX_KERNEL_NAME_L2NORMSCALE_INT16TOFP16             "com.vivantecorp.extension.vxcL2NormScale_Int16toFp16"
#define VX_KERNEL_NAME_PARAMETRICRELU                      "com.vivantecorp.extension.vxcParametricRelu"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8                 "com.vivantecorp.extension.vxcParametricRelu_int8"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8_OPT             "com.vivantecorp.extension.vxcParametricRelu_int8_opt"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8_OPT1            "com.vivantecorp.extension.vxcParametricRelu_int8_opt1"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT8_FP16            "com.vivantecorp.extension.vxcParametricRelu_int8_fp16"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_INT16          "com.vivantecorp.extension.vxcParametricReluInt16_Int16"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_INT16_OPT      \
                                "com.vivantecorp.extension.vxcParametricReluInt16_Int16_opt"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_INT16_OPT1     \
                                "com.vivantecorp.extension.vxcParametricReluInt16_Int16_opt1"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_UINT8          "com.vivantecorp.extension.vxcParametricReluUint8_Uint8"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_2D             "com.vivantecorp.extension.vxcParametricRelu_uint8_2d"
#define VX_KERNEL_NAME_PARAMETRICRELU_FP16_UINT8           "com.vivantecorp.extension.vxcParametricReluFp16_Uint8"
#define VX_KERNEL_NAME_PARAMETRICRELU_FP16_INT16           "com.vivantecorp.extension.vxcParametricReluFp16_Int16"
#define VX_KERNEL_NAME_PARAMETRICRELU_INT16_FP16           "com.vivantecorp.extension.vxcParametricReluInt16_Fp16"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_FP16           "com.vivantecorp.extension.vxcParametricReluUint8_Fp16"
#define VX_KERNEL_NAME_PARAMETRICRELU_UINT8_FP16_2D        \
                                "com.vivantecorp.extension.vxcParametricRelu_uint8tofp16_2d"
#define VX_KERNEL_NAME_PARAMETRICRELU_FP16_INT8            "com.vivantecorp.extension.vxcParametricReluFp16_Int8"
#define VX_KERNEL_NAME_PREBBOX                             "com.vivantecorp.extension.preBBoxVXC"
#define VX_KERNEL_NAME_ADD_RELU_KERNEL                     "com.vivantecorp.extension.addRelu"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX                 "com.vivantecorp.extension.poolingWithArgmax"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8            "com.vivantecorp.extension.poolingWithArgmaxInt8"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8_OPT        "com.vivantecorp.extension.poolingWithArgmaxInt8_Int8_opt"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8_INT8       "com.vivantecorp.extension.poolingWithArgmaxInt8_Int8"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16           "com.vivantecorp.extension.poolingWithArgmaxInt16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_INT16     \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_int16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_OPT       \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_s2k2p0_opt"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_FP16      \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT16_AXINT16   \
                                "com.vivantecorp.extension.poolingWithArgmaxInt16_axI16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8           "com.vivantecorp.extension.poolingWithArgmaxUint8_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8_FP16      \
                                "com.vivantecorp.extension.poolingWithArgmaxUint8_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8_FP16_FP16 \
                                "com.vivantecorp.extension.poolingWithArgmaxUint8_fp16_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_INT8_FP16       \
                                "com.vivantecorp.extension.poolingWithArgmaxInt8_fp16_s2k2p0"
#define VX_KERNEL_NAME_POOLING_WITH_ARGMAX_UINT8_2D        "com.vivantecorp.extension.poolingWithArgmaxU8_s2k2p0_2D"
#define VX_KERNEL_NAME_UNPOOLING                           "com.vivantecorp.extension.unpooling"
#define VX_KERNEL_NAME_UNPOOLING_INT8                      "com.vivantecorp.extension.unpoolingInt8"
#define VX_KERNEL_NAME_UNPOOLING_INT8_INT8                 "com.vivantecorp.extension.unpoolingInt8_Int8"
#define VX_KERNEL_NAME_UNPOOLING_INT8_INT8_OPT             "com.vivantecorp.extension.unpoolingInt8_Int8_opt"
#define VX_KERNEL_NAME_UNPOOLING_UINT8                     "com.vivantecorp.extension.unpoolingUint8_Uint8"
#define VX_KERNEL_NAME_UNPOOLING_INT16_INT16               "com.vivantecorp.extension.unpoolingInt16_Int16"
#define VX_KERNEL_NAME_UNPOOLING_INT16_INT16_OPT           "com.vivantecorp.extension.unpoolingInt16_Int16_opt"
#define VX_KERNEL_NAME_UNPOOLING_INT16_INT16_AXINT16       "com.vivantecorp.extension.unpoolingInt16_Int16_axI16"
#define VX_KERNEL_NAME_UNPOOLING_INT16_FP16_AXINT16        "com.vivantecorp.extension.unpoolingInt16_Fp16_axI16"
#define VX_KERNEL_NAME_UNPOOLING_INT16_FP16                "com.vivantecorp.extension.unpoolingInt16_Fp16"
#define VX_KERNEL_NAME_UNPOOLING_FP16_UINT8                "com.vivantecorp.extension.unpoolingFp16_Uint8"
#define VX_KERNEL_NAME_UNPOOLING_FP16_INT8                 "com.vivantecorp.extension.unpoolingFp16_Int8"
#define VX_KERNEL_NAME_UNPOOLING_FP16_INT16                "com.vivantecorp.extension.unpoolingFp16_Int16"
#define VX_KERNEL_NAME_UNPOOLING_FP16FP16_UINT8            "com.vivantecorp.extension.unpoolingFp16Fp16_Uint8"
#define VX_KERNEL_NAME_UNPOOLING_UINT8_FP16                "com.vivantecorp.extension.unpoolingUint8_Fp16"
#define VX_KERNEL_NAME_UNPOOLING_INT8_FP16                 "com.vivantecorp.extension.unpoolingInt8_Fp16"
#define VX_KERNEL_NAME_UNPOOLING_UINT8_2D                  "com.vivantecorp.extension.unpoolingUint8_Uint8_2D"
#define VX_KERNEL_NAME_UNPOOLING_FP16_UINT8_2D             "com.vivantecorp.extension.unpoolingFp16_Uint8_2D"
#define VX_KERNEL_NAME_ARGMAX                              "com.vivantecorp.extension.argMaxVXC"
#define VX_KERNEL_NAME_ARGMAX_INT8                         "com.vivantecorp.extension.argMaxVXCInt8"
#define VX_KERNEL_NAME_ARGMAX_INT8_INT16                   "com.vivantecorp.extension.argMaxVXCInt8_Int16"
#define VX_KERNEL_NAME_ARGMAX_UINT8                        "com.vivantecorp.extension.argMaxVXCUint8"
#define VX_KERNEL_NAME_ARGMAX_INT16                        "com.vivantecorp.extension.argMaxVXCInt16"
#define VX_KERNEL_NAME_ARGMAX_UINT8_INT16                  "com.vivantecorp.extension.argMaxVXCUint8_Int16"
#define VX_KERNEL_NAME_ARGMAX_U8_I16_WXHX256               "com.vivantecorp.extension.argMax_U8_I16_WxHx256"
#define VX_KERNEL_NAME_ARGMAX_I8_I16_WXHX256               "com.vivantecorp.extension.argMax_I8_I16_WxHx256"
#define VX_KERNEL_NAME_ALEXNET_GEMM                        "com.vivantecorp.extension.alexNet_gemmVXC"
#define VX_KERNEL_NAME_IMG2COL_DILATED                     "com.vivantecorp.extension.img2col_dilatedVXC"
#define VX_KERNEL_NAME_IMG2COL_DILATED_INT8                "com.vivantecorp.extension.img2col_dilated_int8VXC"
#define VX_KERNEL_NAME_ALEXNET_GEMM_INT8                   "com.vivantecorp.extension.alexNet_gemm_int8VXC"
#define VX_KERNEL_NAME_ELTWISE_MAX                         "com.vivantecorp.extension.eltwiseMax"
#define VX_KERNEL_NAME_ELTWISE_MAX_INT8                    "com.vivantecorp.extension.eltwiseMax_int8"
#define VX_KERNEL_NAME_ELTWISE_MAX_INT8_NOFL               "com.vivantecorp.extension.eltwiseMax_int8_nofl"
#define VX_KERNEL_NAME_ELTWISE_MAX_INT8FP16_INT8           "com.vivantecorp.extension.eltwiseMax_i8fp16_i8"
#define VX_KERNEL_NAME_ELTWISE_MAX_UINT8FP16_UINT8         "com.vivantecorp.extension.eltwiseMax_u8fp16_u8"
#define VX_KERNEL_NAME_ELTWISE_MAX_UINT8                   "com.vivantecorp.extension.eltwiseMax_uint8"
#define VX_KERNEL_NAME_ELTWISE_MAX_INT16                   "com.vivantecorp.extension.eltwiseMax_int16"
#define VX_KERNEL_NAME_FULLYCONNECTED_AXIS2                "com.vivantecorp.extension.vxcFullyConnected_Axis2"
#define VX_KERNEL_NAME_TENSORCROP_INT16                    "com.vivantecorp.extension.vxcTensorCrop_Int16"
#define VX_KERNEL_NAME_TENSORCROP_INT8                     "com.vivantecorp.extension.vxcTensorCrop_Int8"
#define VX_KERNEL_NAME_TENSORCROP_INT16_FP16               "com.vivantecorp.extension.vxcTensorCrop_Int16_Fp16"
#define VX_KERNEL_NAME_DROPOUT                             "com.vivantecorp.extension.dropoutVXC"
#define VX_KERNEL_NAME_SHUFFLECHANNEL                      "com.vivantecorp.extension.shuffleChannelVXC"
#define VX_KERNEL_NAME_SHUFFLECHANNEL8BITS                 "com.vivantecorp.extension.shuffleChannel8BitsVXC"
#define VX_KERNEL_NAME_RESIZE_16BITS_DOWNSAMPLE_QUARTER    \
                                "com.vivantecorp.extension.resize_16bits_downsample_quarter"
#define VX_KERNEL_NAME_RESIZE_8BITS_DOWNSAMPLE_QUARTER     \
                                "com.vivantecorp.extension.resize_8bits_downsample_quarter"
#define VX_KERNEL_NAME_SCALE_FP16                          "com.vivantecorp.extension.scale_fp16"
#define VX_KERNEL_NAME_TENSORREVERSE                       "com.vivantecorp.extension.tensorReverse_axis0_fp16"
#define VX_KERNEL_NAME_TENSORELU_FP16_2D                   "com.vivantecorp.extension.tensorElu_fp16_2D"
#define VX_KERNEL_NAME_SPACE2DEPTH_INT16_INT16             "com.vivantecorp.extension.vxcReorg2_fp16_fp16_sx2_sy1"
#define VX_KERNEL_NAME_SCALETOTENSOR_FP16                  "com.vivantecorp.extension.ScaletoTensor_Fp16"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT8                  "com.vivantecorp.extension.ScaletoTensor_Int8"
#define VX_KERNEL_NAME_SCALETOTENSOR_FP16_COPY             "com.vivantecorp.extension.ScaletoTensor_Fp16_copy"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT8_COPY             "com.vivantecorp.extension.ScaletoTensor_Int8_copy"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT16                 "com.vivantecorp.extension.ScaletoTensor_Int16"
#define VX_KERNEL_NAME_SCALETOTENSOR_INT16_COPY            "com.vivantecorp.extension.ScaletoTensor_Int16_copy"
#define VX_KERNEL_NAME_SCALETOTENSOR_UINT8                 "com.vivantecorp.extension.ScaletoTensor_UInt8"
#define VX_KERNEL_NAME_SCALETOTENSOR_UINT8_COPY            "com.vivantecorp.extension.ScaletoTensor_UInt8_copy"
#define VX_KERNEL_NAME_GEMM                                "com.vivantecorp.extension.gemm_block4x4_fp16"
#define VX_KERNEL_NAME_GEMM_UINT8                          "com.vivantecorp.extension.gemm_block4x4_u8"
#define VX_KERNEL_NAME_GEMM_FP16U8_Fp16                    "com.vivantecorp.extension.gemm_block4x4_fp16_u8_fp16"
#define VX_KERNEL_NAME_GEMM_FP16U8_U8                      "com.vivantecorp.extension.gemm_block4x4_fp16_u8_u8"
#define VX_KERNEL_NAME_GEMM_TRANSB_FP16U8TOFP16            "com.vivantecorp.extension.gemmTransBFp16U8toFp16"
#define VX_KERNEL_NAME_GEMM_TRANSB_FP16U8TOU8              "com.vivantecorp.extension.gemmTransBFp16U8toU8"
#define VX_KERNEL_NAME_GEMM_TRANSB_U8U8TOFP16              "com.vivantecorp.extension.gemmTransBU8U8toFp16"
#define VX_KERNEL_NAME_LAYERNORM                           "com.vivantecorp.extension.vxcLayerNorm"
#define VX_KERNEL_NAME_LAYERNORM_UINT8                     "com.vivantecorp.extension.vxcLayerNorm_u8"
#define VX_KERNEL_NAME_LAYERNORM_FP16TOU8                  "com.vivantecorp.extension.vxcLayerNormFP16toU8"
#define VX_KERNEL_NAME_INSTANCENORM                        "com.vivantecorp.extension.vxcInstanceNorm"
#define VX_KERNEL_NAME_INSTANCENORM_UINT8                  "com.vivantecorp.extension.vxcInstanceNorm_U8"
#define VX_KERNEL_NAME_INSTANCENORM_INT8                   "com.vivantecorp.extension.vxcInstanceNorm_int8"
#define VX_KERNEL_NAME_INSTANCENORM_INT16                  "com.vivantecorp.extension.vxcInstanceNorm_int16"
#define VX_KERNEL_NAME_INSTANCENORM_UINT8_FP16             "com.vivantecorp.extension.vxcInstanceNormU8_fp16"
#define VX_KERNEL_NAME_INSTANCENORM_INT8_FP16              "com.vivantecorp.extension.vxcInstanceNormInt8_fp16"
#define VX_KERNEL_NAME_INSTANCENORM_INT16_FP16             "com.vivantecorp.extension.vxcInstanceNormInt16_fp16"
#define VX_KERNEL_NAME_INSTANCENORMSUM_UINT8               "com.vivantecorp.extension.vxcInstanceNormSum_u8"
#define VX_KERNEL_NAME_INSTANCENORMSQR_UINT8               "com.vivantecorp.extension.vxcInstanceNormSqr_u8"
#define VX_KERNEL_NAME_INSTANCENORMMEAN_VARI_UINT8         "com.vivantecorp.extension.vxcInstanceNormMeanVari_u8"
#define VX_KERNEL_NAME_TENSORSTACKCONCAT                   "com.vivantecorp.extension.vxcTensorStackConcat"
#define VX_KERNEL_NAME_TENSORSTACKCONCAT8BITS              "com.vivantecorp.extension.vxcTensorStackConcat8Bits"
#define VX_KERNEL_NAME_SIGNALFRAME_WIDTH                   "com.vivantecorp.extension.vxcSignalFrame_width"
#define VX_KERNEL_NAME_SIGNALFRAME_HEIGHT                  "com.vivantecorp.extension.vxcSignalFrame_height"
#define VX_KERNEL_NAME_SIGNALFRAME_CHANNEL                 "com.vivantecorp.extension.vxcSignalFrame_channel"
#define VX_KERNEL_NAME_SIGNALFRAME_WIDTH_8BITS             "com.vivantecorp.extension.vxcSignalFrame_width_8bit"
#define VX_KERNEL_NAME_SIGNALFRAME_HEIGHT_8BITS            "com.vivantecorp.extension.vxcSignalFrame_height_8bit"
#define VX_KERNEL_NAME_SIGNALFRAME_CHANNEL_8BITS           "com.vivantecorp.extension.vxcSignalFrame_channel_8bit"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_INT8             "com.vivantecorp.extension.vxcTensorRelation_Gt_Int8"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_INT8        "com.vivantecorp.extension.vxcTensorRelation_Gte_Int8"
#define VX_KERNEL_NAME_RELATIONAL_LESS_INT8                "com.vivantecorp.extension.vxcTensorRelation_Ls_Int8"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_INT8           "com.vivantecorp.extension.vxcTensorRelation_Lse_Int8"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_INT8            "com.vivantecorp.extension.vxcTensorRelation_Ne_Int8"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_INT16            "com.vivantecorp.extension.vxcTensorRelation_Gt_Int16"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_INT16       "com.vivantecorp.extension.vxcTensorRelation_Gte_Int16"
#define VX_KERNEL_NAME_RELATIONAL_LESS_INT16               "com.vivantecorp.extension.vxcTensorRelation_Ls_Int16"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_INT16          "com.vivantecorp.extension.vxcTensorRelation_Lse_Int16"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_INT16           "com.vivantecorp.extension.vxcTensorRelation_Ne_Int16"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_UINT8            "com.vivantecorp.extension.vxcTensorRelation_Gt_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_UINT8       "com.vivantecorp.extension.vxcTensorRelation_Gte_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_LESS_UINT8               "com.vivantecorp.extension.vxcTensorRelation_Ls_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_UINT8          "com.vivantecorp.extension.vxcTensorRelation_Lse_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_UINT8           "com.vivantecorp.extension.vxcTensorRelation_Ne_Uint8"
#define VX_KERNEL_NAME_RELATIONAL_GREATER_FP16             "com.vivantecorp.extension.vxcTensorRelation_Gt_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_GREATEREQUAL_FP16        "com.vivantecorp.extension.vxcTensorRelation_Gte_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_LESS_FP16                "com.vivantecorp.extension.vxcTensorRelation_Ls_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_LESSEQUAL_FP16           "com.vivantecorp.extension.vxcTensorRelation_Lse_Fp16"
#define VX_KERNEL_NAME_RELATIONAL_NOTEQUAL_FP16            "com.vivantecorp.extension.vxcTensorRelation_Ne_Fp16"
#define VX_KERNEL_NAME_POW_FP16                            "com.vivantecorp.extension.vxcTensorPow_Fp16"
#define VX_KERNEL_NAME_POW_INT16                           "com.vivantecorp.extension.vxcTensorPow_Int16"
#define VX_KERNEL_NAME_POW_INT8                            "com.vivantecorp.extension.vxcTensorPow_Int8"
#define VX_KERNEL_NAME_POW_UINT8                           "com.vivantecorp.extension.vxcTensorPow_Uint8"
#define VX_KERNEL_NAME_FLOORDIV_FP16                       "com.vivantecorp.extension.vxcTensorFloorDiv_Fp16"
#define VX_KERNEL_NAME_FLOORDIV_INT16                      "com.vivantecorp.extension.vxcTensorFloorDiv_Int16"
#define VX_KERNEL_NAME_FLOORDIV_INT8                       "com.vivantecorp.extension.vxcTensorFloorDiv_Int8"
#define VX_KERNEL_NAME_FLOORDIV_UINT8                      "com.vivantecorp.extension.vxcTensorFloorDiv_Uint8"
#define VX_KERNEL_NAME_MINIMUM_F16TOF16                    "com.vivantecorp.extension.vxcTensorMinimum_F16toF16"
#define VX_KERNEL_NAME_MINIMUM_F16TOF16_2D                 "com.vivantecorp.extension.vxcTensorMinimum_F16toF16_2D"
#define VX_KERNEL_NAME_MINIMUM_I8TOI8                      "com.vivantecorp.extension.vxcTensorMinimum_I8toI8"
#define VX_KERNEL_NAME_MINIMUM_I8TOI8_2D                   "com.vivantecorp.extension.vxcTensorMinimum_I8toI8_2D"
#define VX_KERNEL_NAME_MINIMUM_I8FP16TOI8                  "com.vivantecorp.extension.vxcTensorMinimum_I8Fp16toI8"
#define VX_KERNEL_NAME_MINIMUM_I8FP16TOI8_2D               "com.vivantecorp.extension.vxcTensorMinimum_I8Fp16toI8_2D"
#define VX_KERNEL_NAME_MINIMUM_I8FP16TOFP16                "com.vivantecorp.extension.vxcTensorMinimum_I8Fp16toFp16"
#define VX_KERNEL_NAME_MINIMUM_I8FP16TOFP16_2D             "com.vivantecorp.extension.vxcTensorMinimum_I8Fp16toFp16_2D"
#define VX_KERNEL_NAME_MINIMUM_U8FP16TOFP16                "com.vivantecorp.extension.vxcTensorMinimum_U8Fp16toFp16"
#define VX_KERNEL_NAME_MINIMUM_U8FP16TOFP16_2D             "com.vivantecorp.extension.vxcTensorMinimum_U8Fp16toFp16_2D"
#define VX_KERNEL_NAME_MINIMUM_U8TOU8                      "com.vivantecorp.extension.vxcTensorMinimum_U8toU8"
#define VX_KERNEL_NAME_MINIMUM_U8TOU8_2D                   "com.vivantecorp.extension.vxcTensorMinimum_U8toU8_2D"
#define VX_KERNEL_NAME_MINIMUM_I16TOI16                    "com.vivantecorp.extension.vxcTensorMinimum_I16toI16"
#define VX_KERNEL_NAME_MINIMUM_I16TOI16_2D                 "com.vivantecorp.extension.vxcTensorMinimum_I16toI16_2D"
#define VX_KERNEL_NAME_SPATIAL_TRANSFORMER                 "com.vivantecorp.extension.vxcTransform_Gemm_F16toF16"
#define VX_KERNEL_NAME_TRANSFORM_SETUP_THRES_F16TOF16      "com.vivantecorp.extension.vxcTransform_setupThres_F16toF16"
#define VX_KERNEL_NAME_TRANSFORM_INTERP_F16TOF16_2D        "com.vivantecorp.extension.vxcTransform_InterP_F16toF16_2D"
#define VX_KERNEL_NAME_TRANSFORM_INTERP_F16TOF16           "com.vivantecorp.extension.vxcTransform_InterP_F16toF16"
#define VX_KERNEL_NAME_LOGICAL_OR_INT16                    "com.vivantecorp.extension.vxcTensorLogical_or_int16"
#define VX_KERNEL_NAME_LOGICAL_OR_INT8                     "com.vivantecorp.extension.vxcTensorLogical_or_int8"
#define VX_KERNEL_NAME_LOGICAL_OR_UINT8                    "com.vivantecorp.extension.vxcTensorLogical_or_uint8"
#define VX_KERNEL_NAME_SELECT_INT8                         "com.vivantecorp.extension.vxcTensorSelect_int8"
#define VX_KERNEL_NAME_SELECT_UINT8                        "com.vivantecorp.extension.vxcTensorSelect_uint8"
#define VX_KERNEL_NAME_SELECT_INT16                        "com.vivantecorp.extension.vxcTensorSelect_int16"
#define VX_KERNEL_NAME_LSTMUNIT_ACTIVATION                 "com.vivantecorp.extension.vxcLSTMUnit_Activation_SW"
/* lstm unit activation: layer norm */
#define VX_KERNEL_NAME_LSTMUNIT_CLP_F16toF16_CELL_F32   "com.vivantecorp.extension.vxcLSTMUnit_CLP_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CLP_F16toF16_CELL_F16   "com.vivantecorp.extension.vxcLSTMUnit_CLP_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_LP_F16toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_LP_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_LP_F16toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_LP_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toI16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toI16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toI8_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toI8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toU8_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toI16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toI16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toI8_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toI8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CL_F16toU8_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CL_F16toU8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_L_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toI16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_L_F16toI16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toI8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_L_F16toI8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toU8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_L_F16toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_L_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toI16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_L_F16toI16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toI8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_L_F16toI8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_L_F16toU8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_L_F16toU8_CELL_F32"
/* lstm unit activation: hybrid fp16  */
#define VX_KERNEL_NAME_LSTMUNIT_BP_F16toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_BP_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_BP_F16toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_BP_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_B_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toI16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_B_F16toI16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toI8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_B_F16toI8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toU8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_B_F16toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_B_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toI16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_B_F16toI16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toI8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_B_F16toI8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_B_F16toU8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_B_F16toU8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CBP_F16toF16_CELL_F32   "com.vivantecorp.extension.vxcLSTMUnit_CBP_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CBP_F16toF16_CELL_F16   "com.vivantecorp.extension.vxcLSTMUnit_CBP_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toI16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toI16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toI8_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toI8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toU8_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toI16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toI16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toI8_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toI8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CB_F16toU8_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CB_F16toU8_CELL_F32"
/* lstm unit activation: hybrid u8  */
#define VX_KERNEL_NAME_LSTMUNIT_BP_U8toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_BP_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_BP_U8toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_BP_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_U8toF16_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_B_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_U8toU8_CELL_F16       "com.vivantecorp.extension.vxcLSTMUnit_B_U8toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_B_U8toF16_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_B_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_B_U8toU8_CELL_F32       "com.vivantecorp.extension.vxcLSTMUnit_B_U8toU8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CBP_U8toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CBP_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CBP_U8toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CBP_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_U8toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CB_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_U8toU8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_CB_U8toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CB_U8toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CB_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CB_U8toU8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_CB_U8toU8_CELL_F32"
/* lstm unit activation: standard_fp16 */
#define VX_KERNEL_NAME_LSTMUNIT_SP_F16toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_SP_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_SP_F16toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_SP_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_S_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toI16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_S_F16toI16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toI8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_S_F16toI8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toU8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_S_F16toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_S_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toI16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_S_F16toI16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toI8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_S_F16toI8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_S_F16toU8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_S_F16toU8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CSP_F16toF16_CELL_F32   "com.vivantecorp.extension.vxcLSTMUnit_CSP_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CSP_F16toF16_CELL_F16   "com.vivantecorp.extension.vxcLSTMUnit_CSP_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toI16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toI16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toI8_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toI8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toU8_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toI16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toI16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toI8_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toI8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CS_F16toU8_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CS_F16toU8_CELL_F32"
/* lstm unit activation: standard_u8 */
#define VX_KERNEL_NAME_LSTMUNIT_SP_U8toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_SP_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_SP_U8toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_SP_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_U8toF16_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_S_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_U8toU8_CELL_F16       "com.vivantecorp.extension.vxcLSTMUnit_S_U8toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_S_U8toF16_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_S_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_S_U8toU8_CELL_F32       "com.vivantecorp.extension.vxcLSTMUnit_S_U8toU8_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CSP_U8toF16_CELL_F32    "com.vivantecorp.extension.vxcLSTMUnit_CSP_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CSP_U8toF16_CELL_F16    "com.vivantecorp.extension.vxcLSTMUnit_CSP_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_U8toF16_CELL_F16     "com.vivantecorp.extension.vxcLSTMUnit_CS_U8toF16_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_U8toU8_CELL_F16      "com.vivantecorp.extension.vxcLSTMUnit_CS_U8toU8_CELL_F16"
#define VX_KERNEL_NAME_LSTMUNIT_CS_U8toF16_CELL_F32     "com.vivantecorp.extension.vxcLSTMUnit_CS_U8toF16_CELL_F32"
#define VX_KERNEL_NAME_LSTMUNIT_CS_U8toU8_CELL_F32      "com.vivantecorp.extension.vxcLSTMUnit_CS_U8toU8_CELL_F32"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_FP16     "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_fp16"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_U8_FP16  "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_u8_fp16"
#define VX_KERNEL_NAME_TENSORADD_MEAN_STDDEV_NORM_I16_FP16 "com.vivantecorp.extension.vxcTensorAddMeanStdNorm_i16_fp16"
#define VX_KERNEL_NAME_STACK                               "com.vivantecorp.extension.vxcStack"

/*! \brief The Example Library Set
 * \ingroup group_example_ext
 */
#define VX_LIBRARY_LIBNNEXT (0x3) // assigned from Khronos, vendors control their own

/*! \brief The list of Example Kernels.
 * \ingroup group_xyz_ext
 */
//! [KERNEL ENUM]
enum vx_kernel_libnnext_ext_e
{
    /*! \brief The Example Kernel */
    VX_KERNEL_ENUM_LIBNNEXT             =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LIBNNEXT_OFFSET,
    VX_KERNEL_ENUM_PERMUTECWH           =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PREMUTE_OFFSET,
    VX_KERNEL_ENUM_PERMUTECHW           =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PREMUTE_OFFSET + 1,
    VX_KERNEL_ENUM_PRIORBOX             =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PRIORBOX_OFFSET,
    VX_KERNEL_ENUM_FLATTEN              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_FLATTEN_OFFSET,
    VX_KERNEL_ENUM_L2NORMALIZESCALE     =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_L2NORMALIZESCALE_OFFSET,
    VX_KERNEL_ENUM_L2NORMSCALE_SUMRSQRT =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_L2NORMALIZESCALE_OFFSET + 1,
    VX_KERNEL_ENUM_L2NORMSCALE_MULSCALE =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_L2NORMALIZESCALE_OFFSET + 2,
    VX_KERNEL_ENUM_PARAMETRICRELU       =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PARAMETRICRELU_OFFSET,
    VX_KERNEL_ENUM_PREBBOX              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_PREBBOX_OFFSET,
    VX_KERNEL_ENUM_ADD_RELU_KERNEL      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ADD_RELU_KERNEL_OFFSET,
    VX_KERNEL_ENUM_POOLING_WITH_ARGMAX  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_POOLING_WITH_ARGMAX_OFFSET,
    VX_KERNEL_ENUM_UNPOOLING            =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_UNPOOLING_OFFSET,
    VX_KERNEL_ENUM_ARGMAX               =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ARGMAX_OFFSET,
    VX_KERNEL_ENUM_ALEXNET_GEMM         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ALEXNET_GEMM_OFFSET,
    VX_KERNEL_ENUM_IMG2COL_DILATED      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_IMG2COL_DILATED_OFFSET,
    VX_KERNEL_ENUM_IMG2COL_DILATED_INT8 =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_IMG2COL_DILATED_INT8_OFFSET,
    VX_KERNEL_ENUM_ALEXNET_GEMM_INT8    =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ALEXNET_GEMM_INT8_OFFSET,
    VX_KERNEL_ENUM_ELTWISE_MAX          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_ELTWISE_MAX,
    VX_KERNEL_ENUM_FULLYCONNECTED_AXIS2 =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_FULLYCONNECTED_AXIS2,
    VX_KERNEL_ENUM_TENSORCROP_INT16     =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORCROP_INT16,
    VX_KERNEL_ENUM_TENSORCROP_INT8      =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORCROP_INT8,
    VX_KERNEL_ENUM_TENSORCROP_INT16_FP16 =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORCROP_INT16_FP16,
    VX_KERNEL_ENUM_DROPOUT              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_DROPOUT,
    VX_KERNEL_ENUM_SHUFFLECHANNEL       =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SHUFFLECHANNEL,
    VX_KERNEL_ENUM_RESIZE               =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RESIZE,
    VX_KERNEL_ENUM_REVERSE              =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REVERSE,
    VX_KERNEL_ENUM_RESIZE_16BITS_DOWNSAMPLE_QUARTER =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RESIZE_16BITS_DOWNSAMPLE_QUARTER,
    VX_KERNEL_ENUM_RESIZE_8BITS_DOWNSAMPLE_QUARTER =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RESIZE_8BITS_DOWNSAMPLE_QUARTER,
    VX_KERNEL_ENUM_SCALE                = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SCALE,
    VX_KERNEL_ENUM_TENSORREVERSE        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORREVERSE,
    VX_KERNEL_ENUM_TENSORELU            =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORELU_OFFSET,
    VX_KERNEL_ENUM_SPACE2BATCH          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SPACE2BATCH,
    VX_KERNEL_ENUM_BATCH2SPACE          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_BATCH2SPACE,
    VX_KERNEL_ENUM_SPACE2DEPTH          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SPACE2DEPTH,
    VX_KERNEL_ENUM_IMAGEPROCESS         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_IMAGEPROCESS,
    VX_KERNEL_ENUM_SCALETOTENSOR        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SCALETOTENSOR,
    VX_KERNEL_ENUM_GEMM                 = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_GEMM,
    VX_KERNEL_ENUM_LAYERNORM            = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LAYERNORM,
    VX_KERNEL_ENUM_LAYERNORM_FP16TOU8   =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LAYERNORMFP16TOU8_OFFSET,
    VX_KERNEL_ENUM_REDUCE               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_REDUCE,
    VX_KERNEL_ENUM_INSTANCENORM         =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_INSTANCENORM,
    VX_KERNEL_ENUM_TENSORSTACKCONCAT    =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORSTACKCONCAT,
    VX_KERNEL_ENUM_TENSORSTACKCONCAT8BITS =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSORSTACKCONCAT8BITS_OFFSET,
    VX_KERNEL_ENUM_SIGNALFRAME          =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SIGNALFRAME,
    VX_KERNEL_ENUM_RELATIONALOPS        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_RELATIONALOPS,
    VX_KERNEL_ENUM_POW        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_POW,
    VX_KERNEL_ENUM_FLOORDIV        =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_FLOORDIV,
    VX_KERNEL_ENUM_MINIMUM              = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_MINIMUM,
    VX_KERNEL_ENUM_SPATIAL_TRANSFORMER  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SPATIAL_TRANSFORMER,
    VX_KERNEL_ENUM_LOGICAL_OPS          = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LOGICAL_OPS,
    VX_KERNEL_ENUM_SELECT               = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_SELECT,
    VX_KERNEL_ENUM_LSTMUNIT_ACTIVATION  =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_LSTMUNIT_ACTIVATION,
    VX_KERNEL_ENUM_TENSOR_ADD_MEAN_STDDEV_NORM =
            VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_TENSOR_ADD_MEAN_STDDEV_NORM,
    VX_KERNEL_ENUM_STACK                = VX_KERNEL_BASE(VX_ID_DEFAULT, VX_LIBRARY_LIBNNEXT) + KERNEL_ENUM_STACK,
    // up to 0xFFF kernel enums can be created.
};

/* Assigned from Khronos, for custom */
#define VX_LIBRARY_CUSTOM (0x4)
enum vx_kernel_custom_id_e
{
    _VX_CLIENT_ID_START = VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_CUSTOM ),
#define DEF_OP( name )     VX_CLIENT_ID_##name,
    #include "custom/custom_ops.def"
#undef DEF_OP
};
#define VX_KERNEL_ID( name ) VX_CLIENT_ID_##name

#ifndef gvxOBJ_CHECK
#define gvxOBJ_CHECK(ref) \
    do \
    { \
        status = vxGetStatus((vx_reference)ref); \
        if (ref == 0 || status != VX_SUCCESS) \
        { \
            printf("Obj ERROR: status=%d @ %s(%d)\n", status, __FUNCTION__, __LINE__); \
        } \
    } \
    while (0)
#endif
#ifndef gvxSTATUS_CHECK
#define gvxSTATUS_CHECK(status) \
    do \
    { \
        if (status != VX_SUCCESS) \
        { \
            printf("status ERROR: status=%d @ %s(%d)\n", status, __FUNCTION__, __LINE__); \
        } \
    } \
    while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif

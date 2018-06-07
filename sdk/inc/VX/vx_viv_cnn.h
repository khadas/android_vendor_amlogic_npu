/****************************************************************************
*
*    Copyright 2012 - 2018 Vivante Corporation, Santa Clara, California.
*    All Rights Reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining
*    a copy of this software and associated documentation files (the
*    'Software'), to deal in the Software without restriction, including
*    without limitation the rights to use, copy, modify, merge, publish,
*    distribute, sub license, and/or sell copies of the Software, and to
*    permit persons to whom the Software is furnished to do so, subject
*    to the following conditions:
*
*    The above copyright notice and this permission notice (including the
*    next paragraph) shall be included in all copies or substantial
*    portions of the Software.
*
*    THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
*    IN NO EVENT SHALL VIVANTE AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
*    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/



#ifndef _VX_VIV_CNN_H_
#define _VX_VIV_CNN_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum viv_nn_op_type_e
{
    VIV_NN_OP_S8 = 0,
    VIV_NN_OP_F16 = 1,
    VIV_NN_OP_S16 = 2,
    VIV_NN_OP_F32 = 3
}viv_nn_op_type_e;

typedef enum viv_nn_pooling_type_e
{
    VIV_NN_POOLING_NON = 0,
    VIV_NN_POOLING_MAX = 1,
    VIV_NN_POOLING_AVG = 2,
    VIV_NN_POOLING_FIRST_PIXEL = 3
}viv_nn_pooling_type_e;

typedef enum viv_nn_norm_type_e
{
    VIV_NN_NORM_SAME_MAP = 0,
    VIV_NN_NORM_ACROSS_MAPS = 1,
}viv_nn_norm_type_e;

typedef enum viv_nn_non_linear_func_e
{
    VIV_NN_NONLINEAR_NON = 0,
    VIV_NN_NONLINEAR_LOGISTIC = 1,
    VIV_NN_NONLINEAR_HYPERBOLIC_TAN = 2,
    VIV_NN_NONLINEAR_RELU = 3,
    VIV_NN_NONLINEAR_BRELU = 4,
    VIV_NN_NONLINEAR_SOFTRELU = 5,
    VIV_NN_NONLINEAR_ABS = 6,
    VIV_NN_NONLINEAR_SQUARE = 7,
    VIV_NN_NONLINEAR_SQRT = 8,
    VIV_NN_NONLINEAR_LINEAR = 9
}viv_nn_non_linear_func_e;

typedef enum _vx_nn_layer_type_e
{
    VIV_NN_INVALID_LAYER,
    VIV_NN_CONVOLUTION_LAYER    = 0x1,
    VIV_NN_FULL_CONNECTED_LAYER = 0x2,
    VIV_NN_SOFTMAX_LAYER        = 0x4,
    VIV_NN_RELU_LAYER           = 0x8,
    VIV_NN_POOLING_LAYER        = 0x10,
    VIV_NN_LRN_LAYER            = 0x20,
}vx_nn_layer_type_e;

typedef struct _viv_nn_tensor_op_s
{
    /* Raw filter weight and bias data in float32 type */
    const float *weightData;
    const float *biasData;
    unsigned int weightDataSize;
    unsigned int biasDataSize;
    const void  *weightObj;
    const void  *biasObj;


    /* Dimensions */
    unsigned int    filterDimensions[6];            /* order: x, y, z, w: row, col, slice, filter. 6-D is reserved for locally-connected*/
    unsigned int    inputDimensions[4];             /* order: x, y, z, batch */
    unsigned int    outputDimensions[4];            /* order: x, y, z, batch */

    unsigned int    pad;

    /* Operation Type */
    viv_nn_op_type_e    opType;

    /* stride */
    unsigned int    stride[2];                      /* x, y stride */

    /* group */
    unsigned int     group;

    /* non-linear */
    viv_nn_non_linear_func_e nonlinearFunc;

    /* pooling */
    viv_nn_pooling_type_e   poolingType;
    unsigned int            poolingSize;
    unsigned int            poolingStride;

    /* normal */
    viv_nn_norm_type_e      normalType;
    unsigned int            normalSize;
    float                   normalA;
    float                   normalB;

    unsigned int            layerType;
} viv_nn_tensor_op_s;

#define VIV_NN_MAX_CONCATENATE  (16)
typedef struct _viv_nn_concatenate_op_s
{
    /* Dimensions */
    unsigned int    inputDimensions[VIV_NN_MAX_CONCATENATE][4];             /* For each input, order: x, y, z, batch */
    unsigned int    outputDimensions[4];                                    /* Order: x, y, z, batch */

    /* To extend : add sub view description */

} viv_nn_concatenate_op_s;

typedef enum _viv_nn_attribute_e
{
    VIV_NN_ATTRIBUTE_PROTO_SHAPE = 0,
}
viv_nn_attribute_e;

typedef struct _input_shape
{
    unsigned int width;
    unsigned int height;
    unsigned int type;
    unsigned int batch;
}
input_shape;

typedef struct _output_shape
{
    unsigned int type;
    unsigned int numbs;
}
output_shape;

typedef struct _viv_cnn_shape
{
    input_shape input;
    output_shape output;

} viv_cnn_shape;


VX_API_ENTRY vx_status VX_API_CALL vx_vivGenerateNodeTensorOp(vx_node node, vx_uint32 level, viv_nn_tensor_op_s* pTensorOp);
VX_API_ENTRY vx_status VX_API_CALL vx_vivConfigCNNNodeFromIR(vx_node node, vx_char* sIRfoldername, vx_size batchSize, viv_nn_op_type_e nnType);
VX_API_ENTRY vx_status VX_API_CALL vx_vivConfigCNNNodeFromCaffe(vx_node node, char* sCaffeModelName, char* sCaffePrototxtName, vx_uint32 batchSize, viv_nn_op_type_e nnType);
VX_API_ENTRY vx_status VX_API_CALL vx_vivConfigCNNNodeFromTensorFlow(vx_node node, char* sTSnPyName, char* sTSPyName);

VX_API_ENTRY vx_status VX_API_CALL vx_vivQueryCNN(vx_char* prototxt, viv_nn_attribute_e type, void * shape);

VX_API_ENTRY vx_status VX_API_CALL vx_vivConcatenate(vx_array* pInputs, vx_uint32 numInputs, vx_array outputs, viv_nn_concatenate_op_s* pConcatenateOp);

#ifdef __cplusplus
}
#endif
#endif

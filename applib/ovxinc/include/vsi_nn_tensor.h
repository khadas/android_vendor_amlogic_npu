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
#ifndef _VSI_NN_TENSOR_H
#define _VSI_NN_TENSOR_H

#include "vsi_nn_platform.h"
#include "vsi_nn_types.h"

#define VSI_NN_MAX_DIM_NUM              (4)
#define VSI_NN_TENSOR_ID_NA             ((uint32_t)-1)
#define VSI_NN_TENSOR_ID_AUTO           (VSI_NN_TENSOR_ID_NA - 1)
#define VSI_NN_DIM_AUTO                 (0)

typedef enum _vsi_nn_dim_fmt_e
{
    VSI_NN_DIM_FMT_NCHW = 0x00,
    VSI_NN_DIM_FMT_NHWC = 0x01,
    VSI_NN_DIM_FMT_NA   = 0xFF,
    VSI_NN_DIM_FMT_AUTO = VSI_NN_DIM_FMT_NA - 1,
} vsi_nn_dim_fmt_e;

typedef enum
{
    /* none quantized */
    VSI_NN_QNT_TYPE_NONE = 0,
    /* dynamic fixed point */
    VSI_NN_QNT_TYPE_DFP = VX_QUANT_DYNAMIC_FIXED_POINT,
    /* affine asymmetric */
    VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC = VX_QUANT_AFFINE_SCALE,
    /* undefined type */
    VSI_NN_QNT_TYPE_NA = 0xff,
} vsi_nn_qnt_type_e;

typedef struct vsi_nn_dtype
{
    vsi_nn_dim_fmt_e  fmt;
    vsi_nn_type_e     vx_type;
    struct
    {
        vsi_nn_qnt_type_e qnt_type;
        union
        {
            /* Meanful in dynamic fixed point */
            struct
            {
                int8_t  fl;
            };
            /* Meanful in affine asymmetric */
            struct
            {
                uint32_t  zero_point;
                float     scale;
            };
        };
    };
} vsi_nn_dtype_t;

typedef struct vsi_nn_tensor_attr
{
    uint32_t   size[VSI_NN_MAX_DIM_NUM];
    uint32_t   dim_num;
    /* Virtual tensor*/
    vsi_bool     vtl;
    vsi_bool     is_const;
    vsi_nn_dtype_t dtype;
} vsi_nn_tensor_attr_t;

struct _vsi_nn_tensor
{
    /* Tensor attributes */
    vsi_nn_tensor_attr_t attr;
    /* OVX tensor */
    vx_tensor t;
    /* Optimized weight bias tensor */
    vx_weights_biases_parameter wb;
};

typedef struct _vsi_nn_tensor_rel_table
{
    vsi_nn_node_id_t node;
    uint32_t         index;
}vsi_nn_tensor_rel_table_t;

typedef struct _vsi_nn_tensor_rel
{
    struct
    {
        vsi_nn_tensor_rel_table_t *table;
        uint32_t                   num;
    } input;
    struct
    {
        vsi_nn_tensor_rel_table_t *table;
        uint32_t                   num;
    } output;
}vsi_nn_tensor_rel_t;

#endif


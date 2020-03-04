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
#ifndef _VSI_NN_OP_CLIENT_POW_H
#define _VSI_NN_OP_CLIENT_POW_H

#include "vsi_nn_types.h"
#include "vsi_nn_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VSI_NN_POW_SH_KERNEL_IDX(_INPUT0_TYPE, _INPUT1_TYPE, _OUTPUT_TYPE) \
    VSI_NN_POW_##_INPUT0_TYPE##_INPUT1_TYPE##TO##_OUTPUT_TYPE##_KERNEL,

enum {
    POW_CPU_KERNEL,

    VSI_NN_POW_SH_KERNEL_IDX(F16, F16, F16)
    VSI_NN_POW_SH_KERNEL_IDX(I16, I16, I16)
    VSI_NN_POW_SH_KERNEL_IDX(I8,  I8,  I8)
    VSI_NN_POW_SH_KERNEL_IDX(U8,  U8,  U8)
    VSI_NN_POW_SH_KERNEL_IDX(U8,  F16, F16)
    VSI_NN_POW_SH_KERNEL_IDX(I8,  F16, F16)
    VSI_NN_POW_SH_KERNEL_IDX(I16, F16, F16)
    VSI_NN_POW_SH_KERNEL_IDX(F16, U8,  F16)
};

enum {
    TENSOR_POW_CPU_KERNEL,

    TENSOR_POW_F16F16TOF16_KERNEL,
    TENSOR_POW_I16I16TOI16_KERNEL,
    TENSOR_POW_I8I8TOI8_KERNEL,
    TENSOR_POW_U8U8TOU8_KERNEL,
    TENSOR_POW_U8F16TOF16_KERNEL,
    TENSOR_POW_I8F16TOF16_KERNEL,
    TENSOR_POW_I16F16TOF16_KERNEL,
    TENSOR_POW_F16U8TOF16_KERNEL,

    TENSOR_POW_KERNEL_COUNTS,
};

enum {
    POWER_INPUT0, //optional
    POWER_INPUT1,

    POWER_INPUTS_COUNT,

    POWER_OUTPUT = 0,

    POWER_OUTPUTS_COUNT,

    POWER_PARAM_COUT = POWER_INPUTS_COUNT + POWER_OUTPUTS_COUNT,
};

#define _VSI_NN_POW_LOCAL_TENSOR_NUM 3

typedef struct _vsi_nn_pow_lcl_data
{
    vx_tensor   local_tensor[_VSI_NN_POW_LOCAL_TENSOR_NUM];
    uint32_t hash_idx;
    vsi_bool execute_on_sw;
} vsi_nn_pow_lcl_data;

typedef struct _vsi_nn_pow_param
{
    /* local data must be the first. */
    vsi_nn_pow_lcl_data local;
} vsi_nn_pow_param;

#ifdef __cplusplus
}
#endif

#endif

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
#ifndef _VSI_NN_UTIL_H
#define _VSI_NN_UTIL_H

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include "vsi_nn_platform.h"
#include "vsi_nn_tensor.h"
#include "vsi_nn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------
        Macros and Variables
-------------------------------------------*/

#ifndef _cnt_of_array
#define _cnt_of_array( arr )            (sizeof( arr )/sizeof( arr[0] ))
#endif

/*-------------------------------------------
                  Functions
-------------------------------------------*/

uint8_t * vsi_nn_LoadBinaryData
    (
    const char * filename,
    uint32_t  * sz
    );

uint32_t vsi_nn_GetStrideSize
    (
    vsi_nn_tensor_attr_t * attr,
    uint32_t            * stride
    );

uint32_t vsi_nn_GetStrideSizeBySize
    (
    uint32_t   * size,
    uint32_t     dim_num,
    vsi_nn_type_e type,
    uint32_t   * stride
    );

uint32_t vsi_nn_GetTotalBytesBySize
    (
    uint32_t   * size,
    uint32_t     dim_num,
    vsi_nn_type_e type
    );

float vsi_nn_DataAsFloat32
    (
    uint8_t    * data,
    vsi_nn_type_e type
    );

void vsi_nn_UpdateTensorDims
    (
    vsi_nn_tensor_attr_t * attr
    );

uint32_t vsi_nn_ComputeFilterSize
    (
    uint32_t   i_size,
    uint32_t   ksize,
    uint32_t * pad,
    uint32_t   stride,
    uint32_t   dilation,
    vsi_nn_round_type_e rounding
    );

void vsi_nn_InitTensorsId
    (
    vsi_nn_tensor_id_t * ids,
    int                  num
    );

void vsi_nn_ComputePadWithPadType
    (
    uint32_t   * in_shape,
    uint32_t     in_dim_num,
    uint32_t   * ksize,
    uint32_t   * stride,
    vsi_nn_pad_e pad_type,
    vsi_nn_round_type_e rounding,
    uint32_t   * out_pad
    );

void vsi_nn_GetPadForOvx
    (
    uint32_t * in_pad,
    uint32_t * out_pad
    );

vsi_bool vsi_nn_CreateTensorGroup
    (
    vsi_nn_graph_t  *  graph,
    vsi_nn_tensor_t *  in_tensor,
    uint32_t          axis,
    vsi_nn_tensor_t ** out_tensors,
    uint32_t          group_number
    );

uint32_t vsi_nn_ShapeToString
    (
    uint32_t * shape,
    uint32_t   dim_num,
    char      * buf,
    uint32_t   buf_sz,
    vsi_bool     for_print
    );

int32_t vsi_nn_Access
    (
    const char *path,
    int32_t mode
    );

int32_t vsi_nn_Mkdir
    (
    const char *path,
    int32_t mode
    );

vsi_bool vsi_nn_CheckFilePath
    (
    const char *path
    );

#ifdef __cplusplus
}
#endif

#endif

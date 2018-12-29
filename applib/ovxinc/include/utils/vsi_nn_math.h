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
#ifndef _VSI_NN_MATH_H
#define _VSI_NN_MATH_H

#include "vsi_nn_types.h"

#define vsi_nn_abs(x)               (((x) < 0)    ? -(x) :  (x))
#define vsi_nn_max(a,b)             ((a > b) ? a : b)
#define vsi_nn_min(a,b)             ((a < b) ? a : b)
#define vsi_nn_clamp(x, min, max)   (((x) < (min)) ? (min) : \
                                 ((x) > (max)) ? (max) : (x))

OVXLIB_API void vsi_nn_Transpose
    (
    uint8_t  * dst,
    uint8_t  * data,
    uint32_t * shape,
    uint32_t   dim_num,
    uint32_t * perm,
    vsi_nn_type_e type
    );

OVXLIB_API void vsi_nn_SqueezeShape
    (
    uint32_t * shape,
    uint32_t * dim_num
    );

OVXLIB_API uint32_t vsi_nn_ShapeProduct
    (
    uint32_t * shape,
    uint32_t   dim_num
    );

//shape: row first <--> column first
OVXLIB_API void vsi_nn_InvertShape
    (
    uint32_t * in,
    uint32_t   dim_num,
    uint32_t * out
    );

//Permute shape: row first <--> column first
OVXLIB_API void vsi_nn_InvertPermuteShape
    (
    uint32_t * in,
    uint32_t   dim_num,
    uint32_t * out
    );

OVXLIB_API double vsi_nn_Rint
    (
    double x
    );

OVXLIB_API float vsi_nn_SimpleRound
    (
    float x
    );
#endif

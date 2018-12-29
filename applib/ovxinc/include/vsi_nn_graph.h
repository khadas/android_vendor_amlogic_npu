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
#ifndef _VSI_NN_GRAPH_H
#define _VSI_NN_GRAPH_H

#include "vsi_nn_platform.h"
#include "vsi_nn_context.h"
#include "vsi_nn_tensor.h"
#include "vsi_nn_node.h"
#include "vsi_nn_ops.h"
#include "vsi_nn_types.h"

#define VSI_NN_MAX_IO_NUM        32

#ifdef __cplusplus
extern "C" {
#endif

struct _vsi_nn_graph
{
    vsi_nn_context_t   ctx;
    /* OpenVX graph */
    vx_graph           g;
    /* Tensor list of this graph */
    vsi_nn_tensor_t ** tensors;
    union
    {
        uint32_t          cur_tid;
        uint32_t          tensor_num;
    };
    uint32_t          max_tensor_num;
    /* Node list of this graph */
    vsi_nn_node_t   ** nodes;
    union
    {
        uint32_t          cur_nid;
        uint32_t          node_num;
    };
    uint32_t          max_node_num;
    uint32_t          max_node_io;
    /* Inputs to the graph */
    struct
    {
        vsi_nn_tensor_id_t * tensors;
        uint32_t            num;
    } input;
    /* Outputs to the graph */
    struct
    {
        vsi_nn_tensor_id_t * tensors;
        uint32_t            num;
    } output;
};

OVXLIB_API vsi_nn_graph_t * vsi_nn_CreateGraph
    (
    vsi_nn_context_t ctx,
    uint32_t        tensor_num,
    uint32_t        node_num
    );

OVXLIB_API void vsi_nn_ReleaseGraph
    (
    vsi_nn_graph_t ** graph
    );

/*
 * Create vx tensor and nodes.
 * */
OVXLIB_API vsi_status vsi_nn_SetupGraph
    (
    vsi_nn_graph_t * graph,
    vsi_bool          sort
    );

/*
 * Call vx verify graph.
 * */
OVXLIB_API vsi_status vsi_nn_VerifyGraph
    (
    vsi_nn_graph_t * graph
    );

OVXLIB_API vsi_status vsi_nn_RunGraph
    (
    vsi_nn_graph_t * graph
    );

OVXLIB_API vsi_nn_tensor_id_t vsi_nn_AddTensor
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_id_t     id,
    vsi_nn_tensor_attr_t * attr,
    /* Optional */
    uint8_t             * data
    );

OVXLIB_API vsi_nn_tensor_id_t vsi_nn_AttachTensorToGraph
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_id_t     id,
    vsi_nn_tensor_t      * tensor
    );

OVXLIB_API void vsi_nn_DeleteTensor
    (
    vsi_nn_graph_t       * graph,
    vsi_nn_tensor_id_t     id
    );

OVXLIB_API vsi_nn_tensor_t * vsi_nn_GetTensor
    (
    vsi_nn_graph_t   * graph,
    vsi_nn_tensor_id_t tensor_id
    );

OVXLIB_API vsi_nn_node_t * vsi_nn_GetNode
    (
    vsi_nn_graph_t   * graph,
    vsi_nn_node_id_t   id
    );

OVXLIB_API void vsi_nn_GetTensors
    (
    vsi_nn_graph_t     * graph,
    vsi_nn_tensor_id_t * tensors_id,
    uint32_t            num,
    vsi_nn_tensor_t   ** tensors
    );

OVXLIB_API vsi_nn_node_t * vsi_nn_AddNode
    (
    vsi_nn_graph_t      * graph,
    vsi_nn_op_t           op,
    uint32_t              input_num,
    uint32_t              output_num,
    vsi_nn_node_id_t    * node_id
    );

OVXLIB_API vsi_nn_node_t * vsi_nn_AppendNode
    (
    vsi_nn_graph_t      * graph,
    vsi_nn_op_t           op,
    vsi_nn_node_id_t    * node_id
    );

OVXLIB_API vsi_bool vsi_nn_SetGraphInputs
    (
    vsi_nn_graph_t      * graph,
    vsi_nn_tensor_id_t  * tensors_id,
    uint32_t             tensor_num
    );

OVXLIB_API vsi_bool vsi_nn_SetGraphOutputs
    (
    vsi_nn_graph_t      * graph,
    vsi_nn_tensor_id_t  * tensors_id,
    uint32_t             tensor_num
    );

OVXLIB_API void vsi_nn_RemoveNode
    (
    vsi_nn_graph_t      * graph,
    vsi_nn_node_id_t      id
    );

OVXLIB_API vsi_nn_node_id_t * vsi_nn_SortGraphNode
    (
    vsi_nn_graph_t * graph
    );

OVXLIB_API uint32_t vsi_nn_GetNodesByUids
    (
    vsi_nn_graph_t   * graph,
    uint32_t        * node_uids,
    uint32_t          node_uids_size,
    vsi_nn_node_id_t * nodes,
    uint32_t          nodes_num
    );

OVXLIB_API void vsi_nn_DumpGraphNodeOutputs
    (
    vsi_nn_graph_t * graph,
    const char     * path,
    uint32_t      *  node_uids,
    uint32_t         node_uids_size,
    vsi_bool         force_fp32,
    vsi_nn_dim_fmt_e data_fmt
    );

OVXLIB_API void vsi_nn_DumpGraphNodeOutputsEx
    (
    vsi_nn_graph_t * graph,
    const char     * path,
    const char     * prefix,
    uint32_t       * node_uids,
    uint32_t         node_uids_size,
    vsi_bool         force_fp32,
    vsi_nn_dim_fmt_e data_fmt
    );

OVXLIB_API void vsi_nn_PrintGraph
    (
    vsi_nn_graph_t * graph
    );

OVXLIB_API void vsi_nn_DumpGraphToJson
    (
    vsi_nn_graph_t *graph
    );

#ifdef __cplusplus
}
#endif

#endif

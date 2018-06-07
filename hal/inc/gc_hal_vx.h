/****************************************************************************
*
*    Copyright (c) 2005 - 2018 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


#ifndef __gc_hal_user_vx_h_
#define __gc_hal_user_vx_h_

#if gcdUSE_VX
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

#if gcdVX_OPTIMIZER > 1
#define gcsVX_KERNEL_PARAMETERS     gcoVX_Hardware_Context

typedef gcoVX_Hardware_Context *    gcsVX_KERNEL_PARAMETERS_PTR;
#else
/* VX kernel parameters. */
typedef struct _gcsVX_KERNEL_PARAMETERS * gcsVX_KERNEL_PARAMETERS_PTR;


typedef struct _gcsVX_KERNEL_PARAMETERS
{
    gctUINT32           kernel;

    gctUINT32           step;

    gctUINT32           xmin;
    gctUINT32           xmax;
    gctUINT32           xstep;

    gctUINT32           ymin;
    gctUINT32           ymax;
    gctUINT32           ystep;

    gctUINT32           groupSizeX;
    gctUINT32           groupSizeY;

    gctUINT32           threadcount;
    gctUINT32           policy;
    gctUINT32           rounding;
    gctFLOAT            scale;
    gctFLOAT            factor;
    gctUINT32           borders;
    gctUINT32           constant_value;
    gctUINT32           volume;
    gctUINT32           clamp;
    gctUINT32           inputMultipleWidth;
    gctUINT32           outputMultipleWidth;

    gctUINT32           order;

    gctUINT32           input_type[10];
    gctUINT32           output_type[10];
    gctUINT32           input_count;
    gctUINT32           output_count;

    gctINT16            *matrix;
    gctINT16            *matrix1;
    gctUINT32           col;
    gctUINT32           row;

    gcsSURF_NODE_PTR    node;

    gceSURF_FORMAT      inputFormat;
    gceSURF_FORMAT      outputFormat;

    gctUINT8            isUseInitialEstimate;
    gctINT32            maxLevel;
    gctINT32            winSize;

    gcoVX_Instructions  instructions;

    vx_evis_no_inst_s   evisNoInst;

    gctUINT32               curDeviceID;
    gctUINT32               usedDeviceCount;
    gcsTHREAD_WALKER_INFO   splitInfo[4];
    gctUINT32               deviceCount;
    gctPOINTER              *devices;

    gctUINT32               optionalOutputs[3];
}
gcsVX_KERNEL_PARAMETERS;
#endif

/******************************************************************************\
****************************** API Declarations *****************************
\******************************************************************************/
gceSTATUS
gcoVX_Initialize(vx_evis_no_inst_s *evisNoInst);

gceSTATUS
gcoVX_BindImage(
    IN gctUINT32            Index,
    IN gcsVX_IMAGE_INFO_PTR Info
    );

gceSTATUS
gcoVX_BindUniform(
    IN gctUINT32            RegAddress,
    IN gctUINT32            Index,
    IN gctUINT32            *Value,
    IN gctUINT32            Num
    );

gceSTATUS
gcoVX_Commit(
    IN gctBOOL Flush,
    IN gctBOOL Stall,
    INOUT gctPOINTER *pCmdBuffer,
    INOUT gctUINT32  *pCmdBytes
    );

gceSTATUS
gcoVX_Replay(
    IN gctPOINTER CmdBuffer,
    IN gctUINT32  CmdBytes
    );

gceSTATUS
gcoVX_InvokeKernel(
    IN gcsVX_KERNEL_PARAMETERS_PTR  Parameters
    );

gceSTATUS
gcoVX_ConstructionInstruction(
    IN gctUINT32_PTR    Point,
    IN gctUINT32        Size,
    IN gctBOOL          Upload,
    OUT gctUINT32_PTR   Physical,
    OUT gctUINT32_PTR   Logical,
    OUT gcsSURF_NODE_PTR* Node
    );

gceSTATUS
gcoVX_AllocateMemory(
    IN gctUINT32        Size,
    OUT gctPOINTER*     Logical,
    OUT gctUINT32*      Physical,
    OUT gcsSURF_NODE_PTR* Node
    );

gceSTATUS
gcoVX_FreeMemory(
    IN gcsSURF_NODE_PTR Node
    );

gceSTATUS
gcoVX_DestroyInstruction(
    IN gcsSURF_NODE_PTR Node
    );

gceSTATUS
gcoVX_DestroyNode(
    IN gcsSURF_NODE_PTR Node
    );

gceSTATUS
gcoVX_KernelConstruct(
    IN OUT gcoVX_Hardware_Context   *Context
    );


gceSTATUS
gcoVX_LockKernel(
    IN OUT gcoVX_Hardware_Context   *Context
    );

gceSTATUS
gcoVX_BindKernel(
    IN OUT gcoVX_Hardware_Context   *Context
    );

gceSTATUS
gcoVX_LoadKernelShader(
    IN gcsPROGRAM_STATE ProgramState
    );

gceSTATUS
gcoVX_InvokeKernelShader(
    IN gcSHADER            Kernel,
    IN gctUINT             WorkDim,
    IN size_t              GlobalWorkOffset[3],
    IN size_t              GlobalWorkScale[3],
    IN size_t              GlobalWorkSize[3],
    IN size_t              LocalWorkSize[3],
    IN gctUINT             ValueOrder
    );

gceSTATUS
gcoVX_Flush(
    IN gctBOOL      Stall
    );

gceSTATUS
gcoVX_TriggerAccelerator(
    IN gctUINT32              CmdAddress,
    IN gceVX_ACCELERATOR_TYPE Type,
    IN gctUINT32              EventId,
    IN gctBOOL                waitEvent
    );

gceSTATUS
gcoVX_ProgrammCrossEngine(
    IN gctPOINTER                Data,
    IN gceVX_ACCELERATOR_TYPE    Type,
    IN gctPOINTER                Options,
    IN OUT gctUINT32_PTR        *Instruction
    );

gceSTATUS
gcoVX_SetNNImage(
    IN gctPOINTER Data,
    IN OUT gctUINT32_PTR *Instruction
    );

gceSTATUS
gcoVX_GetNNConfig(
    IN OUT gctPOINTER Config
    );

gceSTATUS
gcoVX_WaitNNEvent(
    gctUINT32 EventId
    );

gceSTATUS
gcoVX_FlushCache(
    IN gctBOOL      FlushICache,
    IN gctBOOL      FlushPSSHL1Cache,
    IN gctBOOL      FlushNNL1Cache,
    IN gctBOOL      FlushTPL1Cache,
    IN gctBOOL      Stall
    );

gceSTATUS
gcoVX_AllocateMemoryEx(
    IN OUT gctUINT *        Bytes,
    OUT gctUINT32 *         Physical,
    OUT gctPOINTER *        Logical,
    OUT gcsSURF_NODE_PTR *  Node
    );

gceSTATUS
gcoVX_FreeMemoryEx(
    IN gctUINT32            Physical,
    IN gctPOINTER           Logical,
    IN gctUINT              Bytes,
    IN gcsSURF_NODE_PTR     Node
    );

gceSTATUS
gcoVX_GetMemorySize(
    OUT gctUINT32_PTR Size
    );

gceSTATUS
gcoVX_ZeroMemorySize();


gceSTATUS
gcoVX_CreateDevices(
    IN gctUINT     maxDeviceCount,
    IN gctPOINTER   *devices,
    OUT gctUINT    *deviceCount
    );

gceSTATUS
gcoVX_DestroyDevices(
    IN gctUINT      deviceCount,
    IN gctPOINTER   *devices
    );

gceSTATUS
gcoVX_GetCurrentDevice(
    OUT gctPOINTER   *devices
    );

gceSTATUS
gcoVX_SetCurrentDevice(
    IN gctPOINTER   device,
    IN gctINT       deviceID
    );

gceSTATUS
gcoVX_MultiDeviceSync(
    IN gctPOINTER   device
    );


gceSTATUS
gcoVX_SaveContext(
    OUT gcoHARDWARE *Hardware
    );

gceSTATUS
gcoVX_RestoreContext(
    IN gcoHARDWARE Hardware
    );


gceSTATUS
gcoVX_CaptureState(
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *OutputSizeInByte,
    IN gctBOOL Enabled,
    IN gctBOOL dropCommandEnabled
    );


gceSTATUS
gcoVX_SetOCBRemapAddress(
    IN gctUINT32 remapStart,
    IN gctUINT32 remapEnd
    );




#ifdef __cplusplus
}
#endif
#endif
#endif /* __gc_hal_user_vx_h_ */



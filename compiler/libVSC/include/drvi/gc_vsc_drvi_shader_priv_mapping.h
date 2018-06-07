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


#ifndef __gc_vsc_drvi_shader_priv_mapping_h_
#define __gc_vsc_drvi_shader_priv_mapping_h_

BEGIN_EXTERN_C()

/* Forward declarations */
typedef struct _SHADER_CONSTANT_SUB_ARRAY_MAPPING SHADER_CONSTANT_SUB_ARRAY_MAPPING;
typedef struct _SHADER_COMPILE_TIME_CONSTANT      SHADER_COMPILE_TIME_CONSTANT;
typedef struct _SHADER_UAV_SLOT_MAPPING           SHADER_UAV_SLOT_MAPPING;
typedef struct _SHADER_RESOURCE_SLOT_MAPPING      SHADER_RESOURCE_SLOT_MAPPING;
typedef struct _SHADER_SAMPLER_SLOT_MAPPING       SHADER_SAMPLER_SLOT_MAPPING;
typedef struct _SHADER_IO_REG_MAPPING             SHADER_IO_REG_MAPPING;

/* Shader constant static priv-mapping flag */
typedef enum SHS_PRIV_CONSTANT_FLAG
{
    SHS_PRIV_CONSTANT_FLAG_COMPUTE_GROUP_NUM,
    SHS_PRIV_CONSTANT_FLAG_IMAGE_SIZE,
    SHS_PRIV_CONSTANT_FLAG_TEXTURE_SIZE,
    SHS_PRIV_CONSTANT_FLAG_LOD_MIN_MAX,
    SHS_PRIV_CONSTANT_FLAG_LEVELS_SAMPLES,
    SHS_PRIV_CONSTANT_FLAG_BASE_INSTANCE,
    SHS_PRIV_CONSTANT_FLAG_SAMPLE_LOCATION,
    SHS_PRIV_CONSTANT_FLAG_ENABLE_MULTISAMPLE_BUFFERS,
    SHS_PRIV_CONSTANT_FLAG_LOCAL_ADDRESS_SPACE,
    SHS_PRIV_CONSTANT_FLAG_PRIVATE_ADDRESS_SPACE,
    SHS_PRIV_CONSTANT_FLAG_CONSTANT_ADDRESS_SPACE,
    SHS_PRIV_CONSTANT_FLAG_GLOBAL_SIZE,
    SHS_PRIV_CONSTANT_FLAG_LOCAL_SIZE,
    SHS_PRIV_CONSTANT_FLAG_GLOBAL_OFFSET,
    SHS_PRIV_CONSTANT_FLAG_WORK_DIM,
    SHS_PRIV_CONSTANT_FLAG_PRINTF_ADDRESS,
    SHS_PRIV_CONSTANT_FLAG_WORKITEM_PRINTF_BUFFER_SIZE,
    SHS_PRIV_CONSTANT_FLAG_WORK_THREAD_COUNT,
    SHS_PRIV_CONSTANT_FLAG_WORK_GROUP_COUNT,
    SHS_PRIV_CONSTANT_FLAG_LOCAL_MEM_SIZE,
}SHS_PRIV_CONSTANT_FLAG;

/* Shader mem static priv-mapping flag */
typedef enum SHS_PRIV_MEM_FLAG
{
    SHS_PRIV_MEM_FLAG_GPR_SPILLED_MEMORY, /* For gpr register spillage */
    SHS_PRIV_MEM_FLAG_CONSTANT_SPILLED_MEMORY, /* For constant register spillage */
    SHS_PRIV_MEM_FLAG_STREAMOUT_BY_STORE, /* Stream buffer for SO */
    SHS_PRIV_MEM_FLAG_CL_PRIVATE_MEMORY, /* For CL private mem */
    SHS_PRIV_MEM_FLAG_SHARED_MEMORY, /* For CL local memory or DirectCompute shared mem */
    SHS_PRIV_CONSTANT_FLAG_EXTRA_UAV_LAYER,
}SHS_PRIV_MEM_FLAG;

/* !!!!!NOTE: For dynamic (lib-link) patch, the priv-mapping flag will directly use VSC_LIB_LINK_TYPE!!!!! */

/* Definition for common entry of shader priv mapping . */
typedef struct SHADER_PRIV_MAPPING_COMMON_ENTRY
{
    /* Each priv-mapping flag */
    gctUINT                                     privmFlag;

    /* For some flags, there might be multiple mapping, so flag-index to distinguish them.
       It is numbered from zero. */
    gctUINT                                     privmFlagIndex;

    /* For some flags, they will have their private data to tell driver how to do. */
    void*                                       pPrivateData;
}
SHADER_PRIV_MAPPING_COMMON_ENTRY;

/*
  Definition of shader constants priv-mapping
*/

typedef enum SHADER_PRIV_CONSTANT_MODE
{
    SHADER_PRIV_CONSTANT_MODE_CTC           = 0,
    SHADER_PRIV_CONSTANT_MODE_VAL_2_INST    = 1,
    SHADER_PRIV_CONSTANT_MODE_VAL_2_MEMORREG= 2,
}
SHADER_PRIV_CONSTANT_MODE;

typedef struct SHADER_PRIV_CONSTANT_INST_IMM
{
    gctUINT                                     patchedPC;
    gctUINT                                     srcNo;
}
SHADER_PRIV_CONSTANT_INST_IMM;

typedef struct SHADER_PRIV_CONSTANT_CTC
{
    SHADER_COMPILE_TIME_CONSTANT*               pCTC;
    gctUINT                                     hwChannelMask;
}
SHADER_PRIV_CONSTANT_CTC;

typedef struct SHADER_PRIV_CONSTANT_ENTRY
{
    SHADER_PRIV_MAPPING_COMMON_ENTRY            commonPrivm;

    /* Which mode that driver use to flush constant */
    SHADER_PRIV_CONSTANT_MODE                   mode;

    union
    {
        SHADER_CONSTANT_SUB_ARRAY_MAPPING*      pSubCBMapping; /* SHADER_PRIV_CONSTANT_MODE_VAL_2_MEMORREG */
        SHADER_PRIV_CONSTANT_CTC                ctcConstant;   /* SHADER_PRIV_CONSTANT_MODE_CTC */
        SHADER_PRIV_CONSTANT_INST_IMM           instImm;       /* SHADER_PRIV_CONSTANT_MODE_VAL_2_INST */
    } u;
}
SHADER_PRIV_CONSTANT_ENTRY;

typedef struct SHADER_PRIV_CONSTANT_MAPPING
{
    SHADER_PRIV_CONSTANT_ENTRY*                 pPrivmConstantEntries;
    gctUINT                                     countOfEntries;
}
SHADER_PRIV_CONSTANT_MAPPING;

/*
  Definition of shader uav priv-mapping
*/

typedef struct SHADER_PRIV_MEM_DATA_MAPPING
{
    SHADER_COMPILE_TIME_CONSTANT**              ppCTC;
    gctUINT                                     ctcCount;

    SHADER_CONSTANT_SUB_ARRAY_MAPPING**         ppCnstSubArray;
    gctUINT                                     cnstSubArrayCount;
}SHADER_PRIV_MEM_DATA_MAPPING;

typedef struct SHADER_PRIV_UAV_ENTRY
{
    SHADER_PRIV_MAPPING_COMMON_ENTRY            commonPrivm;

    /* The data which will be set to this memory */
    SHADER_PRIV_MEM_DATA_MAPPING                memData;

    SHADER_UAV_SLOT_MAPPING*                    pBuffer;
}
SHADER_PRIV_UAV_ENTRY;

typedef struct SHADER_PRIV_UAV_MAPPING
{
    SHADER_PRIV_UAV_ENTRY*                      pPrivUavEntries;
    gctUINT                                     countOfEntries;
}
SHADER_PRIV_UAV_MAPPING;

/*
  Definition of shader resource priv-mapping
*/

typedef struct SHADER_PRIV_RESOURCE_ENTRY
{
    SHADER_PRIV_MAPPING_COMMON_ENTRY            commonPrivm;
    SHADER_RESOURCE_SLOT_MAPPING*               pSrv;
}
SHADER_PRIV_RESOURCE_ENTRY;

typedef struct SHADER_PRIV_RESOURCE_MAPPING
{
    SHADER_PRIV_RESOURCE_ENTRY*                 pPrivResourceEntries;
    gctUINT                                     countOfEntries;
}
SHADER_PRIV_RESOURCE_MAPPING;

/*
  Definition of shader sampler priv-mapping
*/

typedef struct SHADER_PRIV_SAMPLER_ENTRY
{
    SHADER_PRIV_MAPPING_COMMON_ENTRY            commonPrivm;
    SHADER_SAMPLER_SLOT_MAPPING*                pSampler;
}
SHADER_PRIV_SAMPLER_ENTRY;

typedef struct SHADER_PRIV_SAMPLER_MAPPING
{
    SHADER_PRIV_SAMPLER_ENTRY*                  pPrivSamplerEntries;
    gctUINT                                     countOfEntries;
}
SHADER_PRIV_SAMPLER_MAPPING;

/*
  Definition of shader output priv-mapping
*/

typedef struct SHADER_PRIV_OUTPUT_ENTRY
{
    SHADER_PRIV_MAPPING_COMMON_ENTRY            commonPrivm;
    SHADER_IO_REG_MAPPING*                      pOutput;
}
SHADER_PRIV_OUTPUT_ENTRY;

typedef struct SHADER_PRIV_OUTPUT_MAPPING
{
    SHADER_PRIV_OUTPUT_ENTRY*                   pPrivOutputEntries;
    gctUINT                                     countOfEntries;
}
SHADER_PRIV_OUTPUT_MAPPING;

/* Static private mapping table */
typedef struct SHADER_STATIC_PRIV_MAPPING
{
     SHADER_PRIV_CONSTANT_MAPPING               privConstantMapping;
     SHADER_PRIV_UAV_MAPPING                    privUavMapping;
}SHADER_STATIC_PRIV_MAPPING;

/* Dynamic private mapping table */
typedef struct SHADER_DYNAMIC_PRIV_MAPPING
{
     SHADER_PRIV_SAMPLER_MAPPING                privSamplerMapping;
     SHADER_PRIV_OUTPUT_MAPPING                 privOutputMapping;
}SHADER_DYNAMIC_PRIV_MAPPING;

END_EXTERN_C();

#endif /* __gc_vsc_drvi_shader_priv_mapping_h_ */


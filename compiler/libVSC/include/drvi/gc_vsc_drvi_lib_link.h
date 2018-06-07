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


#ifndef __gc_vsc_drvi_lib_link_h_
#define __gc_vsc_drvi_lib_link_h_

BEGIN_EXTERN_C()

typedef union _SPECIALIZATION_CONSTANT_VALUE
{
    gctFLOAT                          fValue[4];
    gctUINT                           uValue[4];
    gctINT                            iValue[4];
}SPECIALIZATION_CONSTANT_VALUE;

typedef struct _VSC_LIB_SPECIALIZATION_CONSTANT
{
    gctCONST_STRING                   varName;
    SPECIALIZATION_CONSTANT_VALUE     value;
    VSC_SHADER_DATA_TYPE              type;
}VSC_LIB_SPECIALIZATION_CONSTANT;

typedef enum _VSC_LIB_LINK_TYPE
{
    /* A general lib link at call site function */
    VSC_LIB_LINK_TYPE_FUNC_NAME                     = 0,

    /*********************************************
        Following are special link points
     *********************************************/

    /* Frag-output color */
    VSC_LIB_LINK_TYPE_COLOR_OUTPUT                  = 1,

    /* Resource access from shader */
    VSC_LIB_LINK_TYPE_RESOURCE                      = 2,

    /* frontfacing is counter clockwise, make to NOT Facing
     * value to make HW happy */
    VSC_LIB_LINK_TYPE_FRONTFACING_CCW               = 3,

    /*
    ** For non-polygon primitive, the value of gl_FrontFacing should be always TRUE,
    ** but for some cases HW can't meet this requirement, so compiler forces change it to TRUE.
    */
    VSC_LIB_LINK_TYPE_FRONTFACING_ALWAY_FRONT       = 4,
    /* image read/write need to use library function to implement
     * the function for given <image, sampler> pair for read,
     * image for write */
    VSC_LIB_LINK_TYPE_IMAGE_READ_WRITE              = 5,
}VSC_LIB_LINK_TYPE;

typedef enum _VSC_RES_OP_BIT
{
    VSC_RES_OP_BIT_TEXLD       = 0x0001,
    VSC_RES_OP_BIT_TEXLD_BIAS  = 0x0002,
    VSC_RES_OP_BIT_TEXLD_LOD   = 0x0004,
    VSC_RES_OP_BIT_TEXLD_GRAD  = 0x0008,
    VSC_RES_OP_BIT_TEXLDP      = 0x0010,
    VSC_RES_OP_BIT_TEXLDP_GRAD = 0x0020,
    VSC_RES_OP_BIT_TEXLDP_BIAS = 0x0040,
    VSC_RES_OP_BIT_TEXLDP_LOD  = 0x0080,
    VSC_RES_OP_BIT_FETCH       = 0x0100,
    VSC_RES_OP_BIT_FETCH_MS    = 0x0200,
    VSC_RES_OP_BIT_GATHER      = 0x0400,
    VSC_RES_OP_BIT_GATHER_PCF  = 0x0800,
    VSC_RES_OP_LODQ            = 0x1000,
}VSC_RES_OP_BIT;

typedef enum _VSC_RES_ACT_BIT
{
    VSC_RES_ACT_BIT_EXTRA_SAMPLER                      = 0x0001,
    VSC_RES_ACT_BIT_REPLACE_SAMPLER_WITH_IMAGE_UNIFORM = 0x0002,
}VSC_RES_ACT_BIT;

typedef enum _VSC_LINK_POINT_RESOURCE_SUBTYPE
{
    VSC_LINK_POINT_RESOURCE_SUBTYPE_TEXLD_EXTRA_LATYER              = 1,
    VSC_LINK_POINT_RESOURCE_SUBTYPE_TEXGRAD_EXTRA_LATYER            = 2,
    VSC_LINK_POINT_RESOURCE_SUBTYPE_TEXFETCH_REPLACE_WITH_IMGLD     = 3,
    VSC_LINK_POINT_RESOURCE_SUBTYPE_TEXGATHER_EXTRA_LAYTER          = 4,
    VSC_LINK_POINT_RESOURCE_SUBTYPE_TEXGATHERPCF_D32F               = 5,
    VSC_LINK_POINT_RESOURCE_SUBTYPE_NORMALIZE_TEXCOORD              = 6,
} VSC_LINK_POINT_RESOURCE_SUBTYPE;

typedef struct _VSC_LIB_LINK_POINT_FUNC_NAME
{
    gctCONST_STRING                   funcName;
}VSC_LIB_LINK_POINT_FUNC_NAME;

typedef struct _VSC_LIB_LINK_POINT_CLR_OUTPUT
{
    gctINT                            location;
    gctUINT                           layers;
}VSC_LIB_LINK_POINT_CLR_OUTPUT;

typedef struct _VSC_LIB_LINK_POINT_RESOURCE
{
    gctUINT                           set;
    gctUINT                           binding;
    gctUINT                           arrayIndex;
    VSC_RES_OP_BIT                    opTypeBits;
    VSC_RES_ACT_BIT                   actBits;
    VSC_LINK_POINT_RESOURCE_SUBTYPE   subType;
} VSC_LIB_LINK_POINT_RESOURCE;

typedef struct _VSC_IMAGE_DESC_INFO
{
    gctINT                          kernelArgNo;    /* kernel arg number for the image */
    VSC_ImageDesc                   imageDesc;      /* image descriptor value */
    gctCONST_STRING                 name;           /* image variable name */
} VSC_IMAGE_DESC_INFO;

typedef struct _VSC_SAMPLER_INFO
{
    gctINT                          kernelArgNo;    /* kernel arg number for the sampler */
    VSC_SamplerValue                sampleValue;    /* sampler descriptor value */
    gctCONST_STRING                 name;           /* sampler variable name */
} VSC_SAMPLER_INFO;


typedef struct _VSC_LIB_LINK_IMAGE_READ_WRITE
{
    gctUINT                         imageCount;
    VSC_IMAGE_DESC_INFO *           imageInfo;
    gctUINT                         samplerCount;
    VSC_SAMPLER_INFO *              samplerInfo;
} VSC_LIB_LINK_IMAGE_READ_WRITE;

typedef struct _VSC_LIB_LINK_POINT
{
    VSC_LIB_LINK_TYPE                 libLinkType;

    /* The interested function maitained by lib shader. NOTE only the interested function
       will be truely linked into main shader at each link point. If this function is NULL,
       then it means all functions in lib might be linked into main shader */
    gctCONST_STRING                   strFunc;

    /* Where the function in lib shader will be linked in main shader */
    union
    {
        VSC_LIB_LINK_POINT_FUNC_NAME  funcName;
        VSC_LIB_LINK_POINT_CLR_OUTPUT clrOutput;
        VSC_LIB_LINK_POINT_RESOURCE   resource;
        VSC_LIB_LINK_IMAGE_READ_WRITE imageReadWrite;
    } u;
}VSC_LIB_LINK_POINT;

typedef struct _VSC_SHADER_LIB_LINK_ENTRY
{
    /* Lib shader */
    SHADER_HANDLE                     hShaderLib;
    /* vreg map from libShader to the current shader */
    void*                             pTempHashTable;

    /* Some variables in function of lib will be determined when function of lib is linking */
    gctUINT                           libSpecializationConstantCount;
    VSC_LIB_SPECIALIZATION_CONSTANT*  pLibSpecializationConsts;

    /* List of link points which designate where the function in lib-shader will be linked
       into main shader. */
    gctUINT                           linkPointCount;
    VSC_LIB_LINK_POINT                linkPoint[1];
}VSC_SHADER_LIB_LINK_ENTRY;

typedef struct _VSC_PROG_LIB_LINK_ENTRY
{
    VSC_SHADER_LIB_LINK_ENTRY         shLibLinkEntry;
    VSC_SHADER_STAGE_BIT              mainShaderStageBits;
}VSC_PROG_LIB_LINK_ENTRY;

typedef struct _VSC_SHADER_LIB_LINK_TABLE
{
    gctUINT                           shLinkEntryCount;
    VSC_SHADER_LIB_LINK_ENTRY*        pShLibLinkEntries;
}VSC_SHADER_LIB_LINK_TABLE;

typedef struct _VSC_PROG_LIB_LINK_TABLE
{
    gctUINT                           progLinkEntryCount;
    VSC_PROG_LIB_LINK_ENTRY*          pProgLibLinkEntries;
}VSC_PROG_LIB_LINK_TABLE;

END_EXTERN_C();

#endif /*__gc_vsc_drvi_lib_link_h_ */



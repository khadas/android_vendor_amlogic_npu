/****************************************************************************
*
*    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


#define LOG_TAG "OvxExecutor"

#include "OvxExecutor.h"
#include "NeuralNetworks.h"

#include <sys/mman.h>

#define NN_TENSOR_MAX_DIMENSION 4

namespace android {
namespace nn {

vx_float32 Fp16toFp32(const vx_uint16 in)
{
    vx_int32 t1;
    vx_int32 t2;
    vx_int32 t3;
    vx_float32 out;

    t1 = in & 0x7fff;                       // Non-sign bits
    t2 = in & 0x8000;                       // Sign bit
    t3 = in & 0x7c00;                       // Exponent

    t1 <<= 13;                              // Align mantissa on MSB
    t2 <<= 16;                              // Shift sign bit into position

    t1 += 0x38000000;                       // Adjust bias

    t1 = (t3 == 0 ? 0 : t1);                // Denormals-as-zero

    t1 |= t2;                               // Re-insert sign bit

    *((uint32_t*)&out) = t1;

    return out;
}

vx_int16 F32toF16(vx_float32 val)
{
    vx_uint32 f32 = (*(vx_uint32 *) &val);
    vx_int16 f16 = 0;
    /* Decode IEEE 754 little-endian 32-bit floating-point value */
    int sign = (f32 >> 16) & 0x8000;
    /* Map exponent to the range [-127,128] */
    int exponent = ((f32 >> 23) & 0xff) - 127;
    int mantissa = f32 & 0x007fffff;
    if (exponent == 128)
    { /* Infinity or NaN */
        if (mantissa)
        {
            /* Flush NaN to 0. */
            f16 = (vx_int16)sign;
        }
        else
        {
            /* Clamp to HALF_MAX/HALF_MIN. */
            f16 = (vx_int16)(sign | ((F16_EXPONENT_BITS - 1) << F16_EXPONENT_SHIFT) | F16_MANTISSA_BITS);
        }
    }
    else if (exponent > 15)
    { /* Overflow - clamp to HALF_MAX/HALF_MIN. */
        f16 = (vx_int16)(sign | ((F16_EXPONENT_BITS - 1) << F16_EXPONENT_SHIFT) | F16_MANTISSA_BITS);
    }
    else if (exponent > -15)
    { /* Representable value */
        /* RTNE */
        int roundingBit = (mantissa >> (F16_MANTISSA_SHIFT - 1)) & 0x1;
        int stickyBits = mantissa & 0xFFF;
        exponent += F16_EXPONENT_BIAS;
        mantissa >>= F16_MANTISSA_SHIFT;
        if (roundingBit)
        {
            if (stickyBits || (mantissa & 0x1))
            {
                mantissa++;
                if (mantissa > F16_MANTISSA_BITS)
                {
                    exponent++;
                    if (exponent > 30)
                    {
                        /* Clamp to HALF_MAX/HALF_MIN. */
                        exponent--;
                        mantissa--;
                    }
                    else
                    {
                        mantissa &= F16_MANTISSA_BITS;
                    }
                }
            }
        }
        f16 = (vx_int16)(sign | exponent << F16_EXPONENT_SHIFT | mantissa);
    }
    else
    {
        f16 = (vx_int16)sign;
    }
    return f16;
}

std::vector<vx_uint32> getStrideFromDims(vx_enum format, vx_uint32* dims, vx_int32 dim_count)
{
    if (format == 0)return std::vector<vx_uint32> (0,0);

    std::vector<vx_uint32> strides;
    strides.resize(dim_count);

    vx_uint32 & stride = strides[0];

    stride = vxcGetTypeSize(format);
    for (int i = 1; i < dim_count; i++)
    {
        strides[i] = dims[i-1] * strides[i-1];
    }

    return strides;
}

std::vector<vx_uint32> convertDims(std::vector<vx_uint32> nhwc)
{
    if (nhwc.size() == 4)
    {
        std::vector<vx_uint32> whcn;

        whcn.push_back(nhwc.at(2));
        whcn.push_back(nhwc.at(1));
        whcn.push_back(nhwc.at(3));
        whcn.push_back(nhwc.at(0));

        return whcn;
    }
    else
        return nhwc;
}


bool setRunTimePoolInfosFromHidlMemories(std::vector<VxRunTimePoolInfo>* poolInfos,
                                         const hidl_vec<hidl_memory>& pools) {
    poolInfos->clear();
    poolInfos->reserve(pools.size());
    bool fail = false;
    for (const auto& pool : pools) {
        poolInfos->emplace_back(pool, &fail);
    }
    if (fail) {
        LOG(ERROR) << "Could not map pools";
        poolInfos->clear();
        return false;
    }
    return true;
}

VxRunTimePoolInfo::VxRunTimePoolInfo(const hidl_memory& hidl_memory, bool* fail) {
    sp<IMemory> m;
    uint8_t* b = nullptr;

    auto memType = hidl_memory.name();
    if (memType == "ashmem") {
        m = mapMemory(hidl_memory);
        if (m == nullptr) {
            LOG(ERROR) << "Can't map shared memory.";
            if (fail) *fail = true;
            return;
        }
        m->update();
        b = reinterpret_cast<uint8_t*>(static_cast<void*>(m->getPointer()));
        if (b == nullptr) {
            LOG(ERROR) << "Can't access shared memory.";
            if (fail) *fail = true;
            return;
        }
    } else if (memType == "mmap_fd") {
        size_t size = hidl_memory.size();
        int fd = hidl_memory.handle()->data[0];
        int prot = hidl_memory.handle()->data[1];
        size_t offset = getSizeFromInts(hidl_memory.handle()->data[2],
                                        hidl_memory.handle()->data[3]);
        b = static_cast<uint8_t*>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));
        if (b == MAP_FAILED) {
            LOG(ERROR) << "VxRunTimePoolInfo::set(): Can't mmap the file descriptor.";
            if (fail) *fail = true;
            return;
        }
    } else {
        LOG(ERROR) << "VxRunTimePoolInfo::set(): unsupported hidl_memory type";
        if (fail) *fail = true;
        return;
    }

    hidlMemory = hidl_memory;
    buffer     = b;
    memory     = m;

}

VxRunTimePoolInfo::VxRunTimePoolInfo(uint8_t* b) {
    buffer = b;
}


/* TODO: short term, make share memory mapping and updating a utility function.
 * TODO: long term, implement mmap_fd as a hidl IMemory service.*/
bool VxRunTimePoolInfo::set(const hidl_memory& hidlMemory) {
    this->hidlMemory = hidlMemory;
    auto memType = hidlMemory.name();
    if (memType == "ashmem") {
        memory = mapMemory(hidlMemory);
        if (memory == nullptr) {
            LOG(ERROR) << "Can't map shared memory.";
            return false;
        }
        memory->update();
        buffer = reinterpret_cast<uint8_t*>(static_cast<void*>(memory->getPointer()));
        if (buffer == nullptr) {
            LOG(ERROR) << "Can't access shared memory.";
            return false;
        }
        return true;
    } else if (memType == "mmap_fd") {
        size_t size = hidlMemory.size();
        int fd = hidlMemory.handle()->data[0];
        int prot = hidlMemory.handle()->data[1];
        size_t offset = getSizeFromInts(hidlMemory.handle()->data[2],
                                        hidlMemory.handle()->data[3]);
        buffer = static_cast<uint8_t*>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));
        if (buffer == MAP_FAILED) {
            LOG(ERROR) << "Can't mmap the file descriptor.";
            return false;
        }
        return true;
    } else {
        LOG(ERROR) << "unsupported hidl_memory type";
        return false;
    }
}

/* Making sure the output data are correctly updated after execution.*/
bool VxRunTimePoolInfo::update() {
    auto memType = hidlMemory.name();
    if (memType == "ashmem") {
        memory->commit();
        return true;
    } else if (memType == "mmap_fd") {
        int prot = hidlMemory.handle()->data[1];
        if (prot & PROT_WRITE) {
            size_t size = hidlMemory.size();
            return msync(buffer, size, MS_SYNC) == 0;
        }
    }
    /* No-op for other types of memory.*/
    return true;
}

vx_status convertDims(vx_uint32 * dims, vx_uint32 *org_dims, vx_uint32 count, bool SNforFC = false)
{
     /* Convert dims from CWHN(NHWC) => WHCN */
    switch (count)
    {
    case 4:
        dims[0] = org_dims[2]; /* W : */
        dims[1] = org_dims[1]; /* H : */
        dims[2] = org_dims[3]; /* C : */
        dims[3] = org_dims[0]; /* N : */
        break;
    case 3:
        dims[0] = org_dims[2]; /* W : */
        dims[1] = org_dims[1]; /* H : */
        dims[2] = org_dims[0]; /* C : */
        dims[3] = 1;           /* N : */
        break;
    case 2:
        {
            if(SNforFC)
            {
            dims[0] = 1;            /* S : */
            dims[1] = 1;            /* N : */
            dims[2] = org_dims[1];  /* C : */
            dims[3] = org_dims[0];  /* N : */
            }
            else
            {
                dims[0] = org_dims[1];
                dims[1] = org_dims[0];
                dims[2] = 1;
                dims[3] = 1;
            }
        }

        break;
    case 1:
        dims[0] = org_dims[0];
        break;
    default:
        break;
    }

    return VX_SUCCESS;
}

template <typename T>
void convertRank_nhwc2whcn(T *org_data, T* dst_data,
                              vx_uint32 whcn_dims[4])
{
    vx_uint32 dim_w = whcn_dims[0];
    vx_uint32 dim_h = whcn_dims[1];
    vx_uint32 dim_c = whcn_dims[2];
    vx_uint32 dim_n = whcn_dims[3];

    for(vx_uint32 n = 0; n < dim_n; n++)
    {
        vx_uint32 block = dim_w * dim_h * dim_c;
        for(vx_uint32 c = 0; c < dim_c; c++)
        {
            vx_uint32 slice = dim_w * dim_h;
            for(vx_uint32 h = 0; h < dim_h; h++)
            {
                for(vx_uint32 w = 0; w < dim_w; w++)
                    dst_data[w + n * block + c * slice + h * dim_w] = org_data[c + n * block + h * dim_w * dim_c + w * dim_c];
            }
        }
    }

}


vx_status copyConstData2Tensor(vx_tensor &tensor , vx_uint8 *dataPtr, vx_enum readOrWrite)
{
    vx_enum type = VX_TYPE_TENSOR;

    VX_CHECK_ERROR(vxQueryReference((vx_reference)tensor, VX_REFERENCE_TYPE, &type, sizeof(vx_enum)));

    if (type == VX_TYPE_TENSOR)
    {
        vx_uint32       output_size[6];
        vx_uint32       stride_size[6];
        vx_tensor_addressing      tensor_addressing = NULL;
        vx_int32        num_of_dims;
        vx_enum         data_format;

        VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_DIMS, output_size, sizeof(output_size)) );
        VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_NUM_OF_DIMS, &num_of_dims, sizeof(num_of_dims)) );
        VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, &data_format, sizeof(data_format)) );

        stride_size[0] = vxcGetTypeSize(data_format);

        for (int i = 1; i < num_of_dims; i++)
        {
            stride_size[i] = stride_size[i-1] * output_size[i - 1];
        }
        tensor_addressing    = vxCreateTensorAddressing(vxGetContext((vx_reference)tensor), output_size, stride_size, num_of_dims);

        vx_bool value = vx_true_e;

        VX_CHECK_ERROR( vxCopyTensorPatch(tensor, NULL, tensor_addressing, dataPtr, readOrWrite, VX_MEMORY_TYPE_HOST) );
        VX_CHECK_ERROR( vxReleaseTensorAddressing(&tensor_addressing) );
        VX_CHECK_ERROR( vxSetTensorAttribute(tensor, VX_TENSOR_VALUE,     &value,     sizeof(vx_bool)) );

        if (tensor_addressing)
            vxReleaseTensorAddressing(&tensor_addressing);
    }

    return VX_SUCCESS;
}

#if !HIGH_PRECISION_COMPUTE

void convertTensorFormatFromFp322Fp16(vx_graph graph, const Operand &operand, VxRunTimeReferenceInfo & to)
{
    vx_enum type = VX_TYPE_TENSOR;

    if( !((operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE ||  operand.lifetime == OperandLifeTime::CONSTANT_COPY) &&
         (operand.type == OperandType::TENSOR_FLOAT32 )
         ) )
         return;

    VX_CHECK_ERROR(vxQueryReference(to.ref, VX_REFERENCE_TYPE, &type, sizeof(vx_enum)));

    if (type == VX_TYPE_TENSOR)
    {

        vx_tensor tensor = (vx_tensor)to.ref;

        vx_enum rank;
        vx_enum precision;
        VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_PRECISION,    &precision,     sizeof(vx_enum)) );
        VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_RANK,    &rank,     sizeof(vx_enum)) );

        if(precision == VX_TENSOR_PRECISION_HIGH)
            return;

        vx_uint32 dataLength = 1;
        for(size_t i = 0; i < to.dimensions.size(); i++)
            dataLength *= to.dimensions[i];


        void *orgData               = malloc(dataLength * vxcGetTypeSize(VX_TYPE_FLOAT32));
        void *formattedData    = malloc(dataLength * vxcGetTypeSize(VX_TYPE_FLOAT16));

        copyConstData2Tensor( (vx_tensor&)to.ref , (vx_uint8 *)orgData, VX_READ_ONLY);

        for(uint32_t i = 0; i < dataLength; i++)
            *((vx_int16 *)formattedData  + i) = F32toF16(((float *) orgData)[i]);

        vx_tensor tensorFp16 = vxCreateTensor(vxGetContext((vx_reference)graph), (vx_size)to.dimensions.size(), to.dimensions.data(), VX_TYPE_FLOAT16, 0);
        copyConstData2Tensor( tensorFp16, (vx_uint8 *)formattedData, VX_WRITE_ONLY);

        vx_enum lifeTime = VX_TENSOR_LIFE_TIME_STATIC;
        VX_CHECK_ERROR( vxSetTensorAttribute(tensorFp16, VX_TENSOR_RANK,            &rank,            sizeof(vx_enum)) );
        VX_CHECK_ERROR( vxSetTensorAttribute(tensorFp16, VX_TENSOR_LIFETIME,       &lifeTime,      sizeof(vx_enum)) );
        VX_CHECK_ERROR( vxSetTensorAttribute(tensorFp16, VX_TENSOR_PRECISION,     &precision,    sizeof(vx_enum)) );

        vxReleaseTensor(&tensor);
        to.ref = (vx_reference)tensorFp16;

        if (orgData)
            free(orgData);

        if formattedData
            free(formattedData);
    }
}
#endif

void convertRankAndFormat(vx_graph graph, const Operand &operand, VxRunTimeReferenceInfo & to, bool convertSNForFC = vx_false_e)
{
    vx_enum type = VX_TYPE_TENSOR;

    VX_CHECK_ERROR(vxQueryReference(to.ref, VX_REFERENCE_TYPE, &type, sizeof(vx_enum)));

    if (type == VX_TYPE_TENSOR)
    {
        vx_tensor   tensor      = (vx_tensor)to.ref;
        vx_context  context     = vxGetContext((vx_reference)graph);
        vx_bool     changed     = vx_false_e;

        vx_bool valuedFlag = vx_true_e;
        vx_enum life_time;
        vxQueryTensor(tensor, VX_TENSOR_LIFETIME, &life_time, sizeof(vx_enum));

        if( (operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE ||  operand.lifetime == OperandLifeTime::CONSTANT_COPY) &&
            (operand.type == OperandType::TENSOR_FLOAT32 || operand.type == OperandType::TENSOR_INT32 || operand.type == OperandType::TENSOR_QUANT8_ASYMM)
          )
        {
            vx_enum         rank, dst_rank;
            vx_enum         precision;
            vx_int32        num_of_dims;
            vx_enum         org_data_format, dst_data_format;
            vx_uint32       tensor_size[NN_TENSOR_MAX_DIMENSION];
            vx_uint32       stride_size[NN_TENSOR_MAX_DIMENSION];
            vx_uint32       convert_tensor_size[NN_TENSOR_MAX_DIMENSION] = {1, 1, 1, 1};

            VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_PRECISION,    &precision,     sizeof(vx_enum)) );
            VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_RANK,         &rank,          sizeof(vx_enum)));
            VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_DIMS,         tensor_size,    sizeof(tensor_size)) );
            VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_NUM_OF_DIMS,  &num_of_dims,   sizeof(num_of_dims)) );
            VX_CHECK_ERROR( vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE,    &org_data_format,   sizeof(org_data_format)) );

            if(rank == VX_TENSOR_RANK_WHCN && precision == VX_TENSOR_PRECISION_HIGH)
                return;

            dst_rank = rank;
            dst_data_format = org_data_format;

            stride_size[0] = 1;
            for (int i = 1; i < num_of_dims; i++)
            {
                stride_size[i] = stride_size[i-1] * tensor_size[i - 1];
            }

            vx_uint32 data_length = 1;
            for(int i = 0; i < num_of_dims; i++)
                data_length *= tensor_size[i];

            void *formattedData = NULL;
            void *orgData = NULL;
            orgData = (void * )malloc(data_length * vxcGetTypeSize(dst_data_format));
            copyConstData2Tensor( (vx_tensor&)to.ref , (vx_uint8 *)orgData, VX_READ_ONLY);

#if !HIGH_PRECISION_COMPUTE
            if(precision != VX_TENSOR_PRECISION_HIGH && org_data_format == VX_TYPE_FLOAT32)
            {
                dst_data_format = VX_TYPE_FLOAT16;
                formattedData  = malloc(data_length * vxcGetTypeSize(dst_data_format));
                for(uint32_t i = 0; i < data_length; i++)
                    *((vx_int16 *)formattedData  + i) = F32toF16(((float *) orgData)[i]);

                changed  = vx_true_e;
            }
            else
#endif
                formattedData = orgData;

            void *rankedData = NULL;
            if(rank == VX_TENSOR_RANK_CWHN || rank == VX_TENSOR_RANK_SN)
            {
                dst_rank = VX_TENSOR_RANK_WHCN;
                convertDims(convert_tensor_size, tensor_size, num_of_dims, convertSNForFC);
                rankedData = malloc(data_length * vxcGetTypeSize(dst_data_format));

                num_of_dims = convertSNForFC ? 4 : num_of_dims;
                if(to.dimensions.size() != (size_t)num_of_dims)
                    to.dimensions.resize(num_of_dims);
                for(int i = 0; i < num_of_dims; i++)
                    to.dimensions[i] = convert_tensor_size[i];

                switch (dst_data_format)
                {
                    case VX_TYPE_FLOAT16:
                        convertRank_nhwc2whcn<vx_uint16>( (vx_uint16 *)formattedData, (vx_uint16 *)rankedData, convert_tensor_size);
                        break;
                    case VX_TYPE_FLOAT32:
                        convertRank_nhwc2whcn<vx_float32>( (vx_float32*)formattedData, (vx_float32*)rankedData, convert_tensor_size);
                        break;
                    case VX_TYPE_UINT32:
                    case VX_TYPE_INT32:
                        convertRank_nhwc2whcn<vx_uint32>( (vx_uint32 *)formattedData, (vx_uint32 *)rankedData, convert_tensor_size);
                        break;
                    case VX_TYPE_UINT8:
                    case VX_TYPE_INT8:
                        convertRank_nhwc2whcn<vx_uint8>( (vx_uint8*)formattedData, (vx_uint8*)rankedData, convert_tensor_size);
                        break;
                    default:
                        LOG(ERROR) << "the data type have not been supported\n";
                        break;
                }
                changed  = vx_true_e;
            }
            else
            {
                memcpy(convert_tensor_size, tensor_size, num_of_dims * sizeof(vx_int32));
                rankedData = formattedData;
            }

            if(changed)
            {
                vx_enum quant_format = ( (dst_data_format == VX_TYPE_UINT8) || (dst_data_format == VX_TYPE_INT32) )? VX_QUANT_AFFINE_SCALE : VX_QUANT_DYNAMIC_FIXED_POINT;
                vx_tensor_create_params_t param = { (vx_uint32)num_of_dims, convert_tensor_size, dst_data_format, quant_format, {{0}}};
                if(quant_format == VX_QUANT_AFFINE_SCALE)
                {
                    param.quant_data.affine.scale     = to.scale;
                    param.quant_data.affine.zeroPoint = to.zeroPoint;
                }
                else
                {
                    param.quant_data.dfp.fixed_point_pos = 0;/*TODO: need to be modify the hard core*/
                }


                vxReleaseTensor(&tensor);
                vx_tensor newtensor = vxCreateTensor2(context, &param, sizeof(vx_tensor_create_params_t));

                VX_CHECK_ERROR( vxSetTensorAttribute(newtensor, VX_TENSOR_RANK,  &dst_rank,  sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute(newtensor, VX_TENSOR_VALUE,  &valuedFlag,  sizeof(vx_bool)) );
                VX_CHECK_ERROR( vxSetTensorAttribute(newtensor, VX_TENSOR_LIFETIME,  &life_time,  sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute(newtensor, VX_TENSOR_PRECISION,  &precision,  sizeof(vx_enum)) );

                to.ref = (vx_reference)newtensor;
                copyConstData2Tensor( (vx_tensor&)to.ref , (vx_uint8*)rankedData, VX_WRITE_ONLY);

                if(rankedData != formattedData)
                    free(rankedData);
                if(formattedData != orgData)
                    free(formattedData);
            }

            if(orgData)
                free(orgData);
        }
    }
    return ;
}



static void convertTensor(const VxRunTimeReferenceInfo& info, vx_uint32 input_dimensionCount)
{
    vx_int32 axisDims_org[4] = { 0 };
    vx_int32 axisDims[4] = { 0 };
    vx_int32 converts[][4] = {
        { 0 },
        { 1, 0 },
        { 2, 1, 0 },
        { 2, 1, 3, 0 },
    };

    copyConstData2Tensor((vx_tensor &)info.ref, (vx_uint8 * )axisDims_org, VX_READ_ONLY);


    for (vx_uint32 i = 0; i < info.dimensions[0]; i++)
    {
        axisDims[i] = axisDims_org[converts[input_dimensionCount - 1][i]];
    }
    copyConstData2Tensor((vx_tensor &)info.ref, (vx_uint8 * )axisDims,VX_WRITE_ONLY);
}

static vx_int32 convertScalar(const VxRunTimeReferenceInfo& info, vx_uint32 input_dimensionCount)
{
    vx_int32 scalar;
    vx_int32 scalar_org;
    VX_CHECK_ERROR(vxCopyScalar((vx_scalar)info.ref, &scalar_org, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    scalar = scalar_org;

    if (input_dimensionCount == 2)
    {
        scalar = ((scalar_org & 1) << 1) | (scalar_org >> 1);
    }
    else if (input_dimensionCount == 3)
    {
        scalar = ((scalar_org & 1) << 2) | (scalar_org & 2) | (scalar_org >> 2);
    }
    else if (input_dimensionCount == 4)
    {
        scalar = ((scalar_org & 4) << 1) | ((scalar_org & 1) << 2) | (scalar_org & 2) | (scalar_org >> 3);
    }


    return scalar;
}



vx_status OvxExecutor::convertScalar2Tensor(VxRunTimeReferenceInfo* info)
{
    vx_context context = vxGetContext(info->ref);
    vx_tensor tensor = nullptr;

    if(isTensor(*info))
        return VX_SUCCESS;

    vx_enum dataType = VX_TYPE_INT32;
    switch(info->type){
        case OperandType::FLOAT32:
            dataType = VX_TYPE_FLOAT32;
            break;
        case OperandType::INT32:
            dataType = VX_TYPE_INT32;
            break;
        case OperandType::UINT32:
            dataType = VX_TYPE_UINT32;
            break;
        default:
            break;
        }

    vx_enum quant_format = (dataType == VX_TYPE_FLOAT32) ? VX_QUANT_DYNAMIC_FIXED_POINT : VX_QUANT_AFFINE_SCALE;
    vx_uint32 size[1] = { 1 }, stride[1] = { info->length };
    vx_tensor_create_params_t param;
    memset((void *)&param, 0, sizeof(param));

    param.sizes = size;
    param.num_of_dims = 1;
    param.data_format = dataType;
    param.quant_format = quant_format;
    if(quant_format == VX_QUANT_AFFINE_SCALE)
    {
        param.quant_data.affine.scale = 1.0f;
        param.quant_data.affine.zeroPoint = 0;
    }

    tensor = vxCreateTensor2(context, &param, sizeof(vx_tensor_create_params_t));
    if (tensor == NULL)
    {
        LOG(ERROR) << "vxCreateTensor failure! at line " << __LINE__;
    }
    vx_tensor_addressing addr = vxCreateTensorAddressing(context, size, stride, 1);
    vxCopyTensorPatch(tensor, NULL, addr, info->buffer, VX_WRITE_ONLY, 0);
    vxReleaseTensorAddressing(&addr);

    vx_enum precision = VX_TENSOR_PRECISION_HIGH;
    vx_enum life_time = VX_TENSOR_LIFE_TIME_STATIC;
    vxSetTensorAttribute(tensor, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum));
    vxSetTensorAttribute(tensor, VX_TENSOR_LIFETIME, &life_time, sizeof(vx_enum));

    if (info->ref)
        vxReleaseScalar((vx_scalar*)&info->ref);

    info->ref = (vx_reference)tensor;


    return VX_SUCCESS;
}


vx_uint8* OvxExecutor::getPointer(Operand operand, const std::vector<VxRunTimePoolInfo> *poolInfos)
{
    vx_uint8* data = NULL;
    /* Gets its value.*/
    switch (operand.lifetime) {
    case OperandLifeTime::CONSTANT_COPY:
        data = (vx_uint8*)(mModel->operandValues.data() + operand.location.offset);
        break;
    case OperandLifeTime::CONSTANT_REFERENCE:
        {
            auto poolIndex = operand.location.poolIndex;
            auto& r = (*poolInfos)[poolIndex];
            data = r.buffer + operand.location.offset;
        }
        break;
    case OperandLifeTime::NO_VALUE:
        break;
    case OperandLifeTime::TEMPORARY_VARIABLE:
        break;
    case OperandLifeTime::MODEL_INPUT:
        break;
    case OperandLifeTime::MODEL_OUTPUT:
        break;
    default:
        LOG(ERROR) << "LIFE TIME : Error lifetime!";
        nnAssert(false);
        break;
    }

    return data;
}

vx_status OvxExecutor::convertAllOperandsToRefences(const std::vector<VxRunTimePoolInfo>* poolInfos, vx_type_e target_format)
{
    vx_status status = VX_SUCCESS;

    const vx_int32 operand_count = mModel->operands.size();


    for (vx_int32 i = 0; i < operand_count; i++)
    {
        Operand operand = mModel->operands[i];
        VxRunTimeReferenceInfo info;
        vx_reference ref = NULL;


        vx_bool value = vx_false_e;
        vx_enum lifetime = VX_TENSOR_LIFE_TIME_DYNAMIC;
        vx_enum precision = VX_TENSOR_PRECISION_AUTO;

        info.lifetime = operand.lifetime;
        info.numberOfUsesLeft = 0;
        info.type = operand.type;
        info.dimensions = operand.dimensions;
        info.length = operand.location.length;
        info.scale = operand.scale;
        info.zeroPoint = operand.zeroPoint;
        info.ref = nullptr;
        info.buffer = nullptr;

        if (info.lifetime != OperandLifeTime::NO_VALUE)
        {
            switch (operand.type)
            {
            case OperandType::FLOAT32:
                info.buffer = getPointer(operand, poolInfos);
                ref = (vx_reference)vxCreateScalar(*mContext, VX_TYPE_FLOAT32, info.buffer);
                break;
            case OperandType::INT32:
                info.buffer = getPointer(operand, poolInfos);
                ref = (vx_reference)vxCreateScalar(*mContext, VX_TYPE_INT32, (vx_int32*)info.buffer);
                break;
            case OperandType::UINT32:
                info.buffer = getPointer(operand, poolInfos);
                ref = (vx_reference)vxCreateScalar(*mContext, VX_TYPE_UINT32, info.buffer);
                break;
            case OperandType::TENSOR_FLOAT32:
            {
                vx_enum rank = (operand.dimensions.size() == 2)?VX_TENSOR_RANK_SN:VX_TENSOR_RANK_CWHN;
                vx_type_e format = target_format;
                info.buffer = getPointer(operand, poolInfos);

                if ((operand.lifetime == OperandLifeTime::MODEL_INPUT) || (operand.lifetime == OperandLifeTime::MODEL_OUTPUT))
                {
                    ref = (vx_reference)vxCreateTensor(*mContext, operand.dimensions.size(), operand.dimensions.data(), format, 0);
                }
                else if (operand.lifetime == OperandLifeTime::TEMPORARY_VARIABLE)
                {

                    ref = (vx_reference)creatVirtualTensorFromRTF(mGraph, info);
                    rank = VX_TENSOR_RANK_WHCN;
                }
                else
                {
                    value = vx_true_e;
                    lifetime = VX_TENSOR_LIFE_TIME_STATIC;

                    vx_tensor_create_params_t param = { (vx_uint32)operand.dimensions.size(), (vx_uint32*)operand.dimensions.data(), format, VX_QUANT_DYNAMIC_FIXED_POINT, {{0}} };

                    std::vector<vx_uint32> stride = getStrideFromDims(VX_TYPE_FLOAT32, operand.dimensions.data(), operand.dimensions.size());

                    /*
                    vx_tensor_addressing addr = vxCreateTensorAddressing(*mContext, operand.dimensions.data(), stride.data(), operand.dimensions.size());
                    ref = (vx_reference)vxCreateTensorFromHandle(*mContext, &param, sizeof(vx_tensor_create_params_t), addr, info.buffer, VX_MEMORY_TYPE_HOST);
                    vxReleaseTensorAddressing(&addr);
                    */

                    ref = (vx_reference)vxCreateTensor2(*mContext, &param, sizeof(vx_tensor_create_params_t));
                    copyConstData2Tensor((vx_tensor &)ref, info.buffer, VX_WRITE_ONLY);

                }

                if (!ref)
                    LOG(ERROR) << "vxCreateTensor for TENSOR_FLOAT32 FAILED! " << format;

                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_RANK, &rank, sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_VALUE,&value, sizeof(vx_bool)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_LIFETIME, &lifetime, sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );

                if (ref)info.ref = ref;

            }
            break;
            case OperandType::TENSOR_INT32:
            {

                vx_enum rank = (operand.dimensions.size() == 2)?VX_TENSOR_RANK_SN:VX_TENSOR_RANK_CWHN;

                info.buffer = getPointer(operand, poolInfos);

                vx_tensor_create_params_t param = { (vx_uint32)operand.dimensions.size(), (vx_uint32*)operand.dimensions.data(), VX_TYPE_INT32, VX_QUANT_AFFINE_SCALE, {{0}}};
                param.quant_data.affine.scale     = operand.scale;
                param.quant_data.affine.zeroPoint = operand.zeroPoint;

                if ((operand.lifetime == OperandLifeTime::MODEL_INPUT) || (operand.lifetime == OperandLifeTime::MODEL_OUTPUT))
                {
                    ref = (vx_reference)vxCreateTensor2(*mContext, &param, sizeof(vx_tensor_create_params_t));
                }
                else if(operand.lifetime == OperandLifeTime::TEMPORARY_VARIABLE)
                {
                    ref = (vx_reference)creatVirtualTensorFromRTF(mGraph, info);
                    rank = VX_TENSOR_RANK_WHCN;
                }
               else
                {
                    value = vx_true_e;
                    lifetime = VX_TENSOR_LIFE_TIME_STATIC;
                    /*
                    vx_tensor_addressing addr = vxCreateTensorAddressing(*mContext, operand.dimensions.data(), getStrideFromDims(VX_TYPE_INT32, operand.dimensions.data(), operand.dimensions.size()).data(), operand.dimensions.size());
                    ref = (vx_reference)vxCreateTensorFromHandle(*mContext, &param, sizeof(vx_tensor_create_params_t), addr, info.buffer, VX_MEMORY_TYPE_HOST);
                    */
                    ref = (vx_reference)vxCreateTensor2(*mContext, &param, sizeof(vx_tensor_create_params_t));
                    copyConstData2Tensor((vx_tensor &)ref, info.buffer, VX_WRITE_ONLY);
                }

                if (!ref)
                    LOG(ERROR) << "vxCreateTensor for TENSOR_INT32 FAILED! ";

                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_RANK, &rank, sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_VALUE,&value, sizeof(vx_bool)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_LIFETIME, &lifetime, sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );

                if (ref)info.ref = ref;
            }
            break;
            case OperandType::TENSOR_QUANT8_ASYMM:
            {
                vx_enum rank = (operand.dimensions.size() == 2)?VX_TENSOR_RANK_SN:VX_TENSOR_RANK_CWHN;
                info.buffer = getPointer(operand, poolInfos);

                vx_tensor_create_params_t param = { (vx_uint32)operand.dimensions.size(), (vx_uint32*)operand.dimensions.data(), VX_TYPE_UINT8, VX_QUANT_AFFINE_SCALE, {{0}}};
                param.quant_data.affine.scale = operand.scale;
                param.quant_data.affine.zeroPoint = operand.zeroPoint;

                if ((operand.lifetime == OperandLifeTime::MODEL_INPUT) || (operand.lifetime == OperandLifeTime::MODEL_OUTPUT))
                {
                    ref = (vx_reference)vxCreateTensor2(*mContext, &param, sizeof(vx_tensor_create_params_t));
                }
                else if (operand.lifetime == OperandLifeTime::TEMPORARY_VARIABLE)
                {
                    ref = (vx_reference)creatVirtualTensorFromRTF(mGraph, info);
                    rank = VX_TENSOR_RANK_WHCN;
                }
                else
                {
                    value = vx_true_e;
                    lifetime = VX_TENSOR_LIFE_TIME_STATIC;

                    /*
                    vx_tensor_addressing addr = vxCreateTensorAddressing(*mContext, operand.dimensions.data(), getStrideFromDims(VX_TYPE_UINT8, operand.dimensions.data(), operand.dimensions.size()).data(), operand.dimensions.size());
                    ref = (vx_reference)vxCreateTensorFromHandle(*mContext, &param, sizeof(vx_tensor_create_params_t), addr, info.buffer, VX_MEMORY_TYPE_HOST);
                    */
                    ref = (vx_reference)vxCreateTensor2(*mContext, &param, sizeof(vx_tensor_create_params_t));
                    copyConstData2Tensor((vx_tensor &)ref, info.buffer, VX_WRITE_ONLY);
                }

                if (!ref)
                    LOG(ERROR) << "vxCreateTensor for TENSOR_QUANT8_ASYMM Failed! scale = " << operand.scale << ", zeroPoint = " << operand.zeroPoint;

                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_RANK, &rank, sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_VALUE,&value, sizeof(vx_bool)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_LIFETIME, &lifetime, sizeof(vx_enum)) );
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );

                if (ref)info.ref = ref;
            }
            break;
            case OperandType::OEM:
            case OperandType::TENSOR_OEM_BYTE:
            default:
                LOG(ERROR) << "Unknown Operand type!";
                break;
            }

            if (ref)
            {

                status = vxGetStatus(ref);
                if (status == VX_SUCCESS)
                    info.ref = ref;
                else
                    LOG(ERROR) << "Reference Error: " << status;
            }
            else
            {
                LOG(ERROR) << "Create reference failed!";
                break;
            }

        }
        else
            LOG(ERROR) << " ********** Operand " << i << " have no value! ********** ";

        mReferenceInfos.push_back(info);
    }

    return status;
}

vx_tensor OvxExecutor::creatVirtualTensorByParam(vx_graph graph, vx_uint32 dimNum, vx_uint32 *dims, OperandType type, vx_float32 scale, int32_t zp)
{
    vx_enum dataType = OperandType::TENSOR_QUANT8_ASYMM == type ? VX_TYPE_UINT8 : (OperandType::TENSOR_INT32 == type ? VX_TYPE_INT32 :
#if HIGH_PRECISION_COMPUTE
        VX_TYPE_FLOAT32
#else
        VX_TYPE_FLOAT16
#endif
            );
    vx_enum quant_format = ( (dataType == VX_TYPE_UINT8) ||  (dataType == VX_TYPE_INT32) )? VX_QUANT_AFFINE_SCALE : 0;
    vx_tensor_create_params_t param = { dimNum, dims, dataType, quant_format, {{0}}};
    param.quant_data.dfp.fixed_point_pos = 0;
    if(quant_format == VX_QUANT_AFFINE_SCALE)
    {
        param.quant_data.affine.scale     = scale;
        param.quant_data.affine.zeroPoint = zp;
    }

   vx_enum precision = VX_TENSOR_PRECISION_AUTO;
   vx_tensor tensor = vxCreateVirtualTensor2(graph, &param, sizeof(vx_tensor_create_params_t));
   VX_CHECK_ERROR( vxSetTensorAttribute(tensor, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );

   return tensor;
}

vx_tensor OvxExecutor::creatTensorByParam(vx_context context, vx_uint32 dimNum, vx_uint32 *dims, OperandType type, vx_float32 scale, int32_t zp)
{
    vx_enum dataType = OperandType::TENSOR_QUANT8_ASYMM == type ? VX_TYPE_UINT8 : (OperandType::TENSOR_INT32 == type ? VX_TYPE_INT32 :
#if HIGH_PRECISION_COMPUTE
        VX_TYPE_FLOAT32
#else
        VX_TYPE_FLOAT16
#endif
            );
    vx_enum quant_format = ( (dataType == VX_TYPE_UINT8) ||  (dataType == VX_TYPE_INT32) )? VX_QUANT_AFFINE_SCALE : 0;
    vx_tensor_create_params_t param = { dimNum, dims, dataType, quant_format, {{0}}};
    param.quant_data.dfp.fixed_point_pos = 0;
    if(quant_format == VX_QUANT_AFFINE_SCALE)
    {
        param.quant_data.affine.scale     = scale;
        param.quant_data.affine.zeroPoint = zp;
    }

   vx_enum precision = VX_TENSOR_PRECISION_AUTO;
   vx_tensor tensor = vxCreateTensor2(context, &param, sizeof(vx_tensor_create_params_t));
   VX_CHECK_ERROR( vxSetTensorAttribute(tensor, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );

   return tensor;
}

vx_tensor OvxExecutor::creatVirtualTensorFromRTF(vx_graph graph, const VxRunTimeReferenceInfo &info)
{
    vx_tensor tensor = NULL;
    vx_uint32 whcnDim[4] = {1,1,1,1};
    if(info.dimensions.size() == 2)
    {
        whcnDim[2] = info.dimensions[1];
        whcnDim[3] = info.dimensions[0];
    }
    else if(info.dimensions.size() == 3)
    {
        whcnDim[0] = info.dimensions[2];
        whcnDim[1] = info.dimensions[1];
        whcnDim[2] = info.dimensions[0];
        whcnDim[3] = 1;
    }
    else if(info.dimensions.size()  == 4)
    {
        whcnDim[0] = info.dimensions[2];
        whcnDim[1] = info.dimensions[1];
        whcnDim[2] = info.dimensions[3];
        whcnDim[3] = info.dimensions[0];
    }

    tensor = creatVirtualTensorByParam(graph, 4, whcnDim, info.type, info.scale, info.zeroPoint);
    if (tensor == NULL)
    {
        LOG(ERROR)<<"fail to creat tensor";
    }

    return tensor;
}

vx_status OvxExecutor::getGraph(const std::vector<VxRunTimePoolInfo>* poolInfos)
{
    vx_status status = VX_SUCCESS;

    if (*mContext == nullptr)
        LOG(ERROR)<<" ----------- Context is NULL ----------- ";

    mGraph = vxCreateGraph(*mContext);
    convertAllOperandsToRefences(poolInfos, VX_TYPE_FLOAT32);
    /*Convert all operaion to corresponding node*/
    const vx_int32 operation_count = mModel->operations.size();

    auto createTempTensor = [&](const VxRunTimeReferenceInfo output, VxRunTimeReferenceInfo act)-> vx_tensor
    {
        vx_enum activation = 0;
        VX_CHECK_ERROR(vxCopyScalar((vx_scalar)act.ref, &activation, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

        vx_tensor tmpTensor = (vx_tensor)output.ref;
        if (FusedActivationFunctionType::kNone != (FusedActivationFunctionType)activation)
        {
            tmpTensor = creatVirtualTensorFromRTF(mGraph, output);
        }
        return tmpTensor;
    };

    auto addActivationNode = [&](const VxRunTimeReferenceInfo output, vx_tensor tempTensor, VxRunTimeReferenceInfo act) -> vx_status
    {
        vx_enum activation = 0;
        VX_CHECK_ERROR(vxCopyScalar((vx_scalar)act.ref, &activation, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

        if (FusedActivationFunctionType::kNone != (FusedActivationFunctionType)activation)
        {
            vx_node node = vxActivationLayer(mGraph, tempTensor, getOvxActivationType((FusedActivationFunctionType)activation), 0, 0, (vx_tensor)output.ref);
            vxReleaseTensor(&tempTensor);
            vxReleaseNode(&node);
        }
        return VX_SUCCESS;
    };


    for (vx_int32 i = 0; i < operation_count; i++)
    {
        VxOperationInfo operation_info;

        Operation operation = mModel->operations[i];
        const hidl_vec<uint32_t>& ins = operation.inputs;
        const hidl_vec<uint32_t>& outs = operation.outputs;

        operation_info.operation = operation;

        LOG(ERROR) << "operation type : " << getOperationName(operation.type);


        /* Function to verify that the number of input and output parameters
         * matches what is expected.  Also checks that all the parameters have
         * values. This function is to be used only for operations that do not
         * accept optional arguments.
         * TODO Have a version that works for optional arguments.
         */
        auto allParametersPresent = [&operation, &ins, &outs, this](size_t requiredIns,
            size_t requiredOuts) -> bool {
            auto verify = [&operation, this](size_t requiredCount, const hidl_vec<uint32_t>& indexes,
                const char* type) -> bool {
                size_t actualCount = indexes.size();
                if (actualCount != requiredCount) {
                    LOG(ERROR) << getOperationName(operation.type)
                        << ": Invalid number of " << type << " operands. Got " << actualCount
                        << " of " << requiredCount;
                    return false;
                }
                for (size_t i = 0; i < actualCount; i++) {
                    if (mModel->operands[indexes[i]].lifetime == OperandLifeTime::NO_VALUE) {
                        LOG(ERROR) << getOperationName(operation.type) << " " << type
                            << " operand " << i << " is required but missing.";
                        return false;
                    }
                }
                return true;
            };
            return verify(requiredIns, ins, "in") && verify(requiredOuts, outs, "out");
        };

        switch (operation.type)
        {
        case OperationType::ADD:
        {
            const VxRunTimeReferenceInfo& in1 = mReferenceInfos[ins[0]];
            const VxRunTimeReferenceInfo& in2 = mReferenceInfos[ins[1]];
            const VxRunTimeReferenceInfo& out = mReferenceInfos[outs[0]];

            if (mGraph)
            {
                vx_tensor tmpTensor = createTempTensor(out, mReferenceInfos[ins[2]]);
                operation_info.node = vxTensorAddNode(mGraph, (vx_tensor)in1.ref, (vx_tensor)in2.ref, VX_CONVERT_POLICY_WRAP, tmpTensor);

                VX_CHECK_ERROR( addActivationNode(out, tmpTensor, mReferenceInfos[ins[2]]) );
            }
            else
                LOG(ERROR) << "mGraph is NULL";
        }
            break;
        case OperationType::MUL:
            {
            const VxRunTimeReferenceInfo& in1 = mReferenceInfos[ins[0]];
            const VxRunTimeReferenceInfo& in2 = mReferenceInfos[ins[1]];
            const VxRunTimeReferenceInfo& out = mReferenceInfos[outs[0]];

            vx_float32 s = 1.0f;
            vx_scalar scale = vxCreateScalar(*mContext, VX_TYPE_FLOAT32, &s);
            VxReferenceInfo ref_info = {(vx_reference)scale, VX_TYPE_SCALAR};
            operation_info.refs.push_back(ref_info);

            if (mGraph)
            {
                vx_tensor tmpTensor = createTempTensor(out, mReferenceInfos[ins[2]]);
                operation_info.node = vxTensorMultiplyNode(mGraph, (vx_tensor)in1.ref, (vx_tensor)in2.ref, scale, VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, tmpTensor);
                VX_CHECK_ERROR( addActivationNode(out, tmpTensor, mReferenceInfos[ins[2]]) );
            }
            else
                LOG(ERROR) << "mGraph is NULL";
            }
            break;

        case OperationType::DEPTHWISE_CONV_2D:
            {
                const size_t inCount = ins.size();
                if ((inCount != 11 && inCount != 8) ||
                    !allParametersPresent(inCount, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                int32_t padding_left = 0, padding_right = 0;
                int32_t padding_top = 0, padding_bottom = 0;
                int32_t stride_width = 0, stride_height = 0;
                int32_t depth_multiplier = 1;

                const VxRunTimeReferenceInfo& input  = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& filter = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& bias   = mReferenceInfos[ins[2]];
                const VxRunTimeReferenceInfo& act    = mReferenceInfos[ins[inCount-1]];

                if (inCount == 11) {

                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &padding_left, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &padding_right, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[5]].ref, &padding_top, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[6]].ref, &padding_bottom, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[7]].ref, &stride_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[8]].ref, &stride_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[9]].ref, &depth_multiplier, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                }
                else {

                    int32_t padding_implicit = 0;

                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &padding_implicit, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &stride_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[5]].ref, &stride_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[6]].ref, &depth_multiplier, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);


                    Shape inputShape = input.shape();
                    Shape filterShape = filter.shape();
                    int32_t input_width = getSizeOfDimension(inputShape, 2);
                    int32_t input_height = getSizeOfDimension(inputShape, 1);
                    int32_t filter_width = getSizeOfDimension(filterShape, 2);
                    int32_t filter_height = getSizeOfDimension(filterShape, 1);
                    calculateExplicitPadding(input_width, stride_width,
                        filter_width, padding_implicit,
                        &padding_left, &padding_right);
                    calculateExplicitPadding(input_height, stride_height,
                        filter_height, padding_implicit,
                        &padding_top, &padding_bottom);

                }

                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];
                Shape outShape = output.shape();

                vx_nn_convolution_params_ext2_t params = {
                    {
                        { (vx_size)padding_left, (vx_size)padding_top, VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR, 0, 0 },
                        (vx_size)padding_right, (vx_size)padding_bottom, VX_PAD_CONSTANT, 0
                        },
                        (vx_uint32)stride_width, (vx_uint32)stride_height, depth_multiplier
                };

                vx_enum precision = VX_TENSOR_PRECISION_HIGH;
                vxSetTensorAttribute((vx_tensor)bias.ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum));

                for(auto &in : ins)
                {
                    convertRankAndFormat(mGraph, mModel->operands[in], mReferenceInfos[in]);
                }

                vx_tensor tmpTensor = createTempTensor(output,act);
                operation_info.node = vxConvolutionLayer(mGraph, (vx_tensor)input.ref, (vx_tensor)filter.ref, (vx_tensor)bias.ref, (const vx_nn_convolution_params_t *)&params, sizeof(vx_nn_convolution_params_ext2_t), tmpTensor);
                VX_CHECK_ERROR( addActivationNode(output, tmpTensor, act) );

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxConvolutionLayer failed!";
                }
            }
            break;

        case OperationType::CONV_2D:
            {
                const size_t inCount = ins.size();
                if ((inCount != 10 && inCount != 7) ||
                    !allParametersPresent(inCount, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                int32_t padding_left = 0, padding_right = 0;
                int32_t padding_top = 0, padding_bottom = 0;
                int32_t stride_width = 0, stride_height = 0;
                int32_t depth_multiplier = 0;

                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& filter = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& bias = mReferenceInfos[ins[2]];

                Shape filterShape = filter.shape();
                int32_t filter_width = getSizeOfDimension(filterShape, 2);
                int32_t filter_height = getSizeOfDimension(filterShape, 1);

                vx_enum precision = VX_TENSOR_PRECISION_HIGH;
                VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)bias.ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum))) ;


                if (inCount == 10) {

                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &padding_left, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &padding_right, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[5]].ref, &padding_top, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[6]].ref, &padding_bottom, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[7]].ref, &stride_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[8]].ref, &stride_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                }
                else {
                    int32_t padding_implicit = 0;

                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &padding_implicit, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &stride_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[5]].ref, &stride_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                    Shape inputShape = input.shape();
                    int32_t input_width = getSizeOfDimension(inputShape, 2);
                    int32_t input_height = getSizeOfDimension(inputShape, 1);
                    calculateExplicitPadding(input_width, stride_width,
                        filter_width, padding_implicit,
                        &padding_left, &padding_right);
                    calculateExplicitPadding(input_height, stride_height,
                        filter_height, padding_implicit,
                        &padding_top, &padding_bottom);

                }

                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                {
                    convertRankAndFormat(mGraph, mModel->operands[ins[1]], mReferenceInfos[ins[1]]);
                    convertRankAndFormat(mGraph, mModel->operands[ins[2]], mReferenceInfos[ins[2]]);
                }

                {
                    vx_tensor convTensor =  createTempTensor(output, mReferenceInfos[ins[ins.size()-1]]);

                    vx_nn_convolution_params_ext2_t params = {
                        {
                            {
                                (vx_size)padding_left, (vx_size)padding_top, VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0
                             },
                            (vx_size)padding_right, (vx_size)padding_bottom, VX_PAD_CONSTANT, 0
                        },
                        (vx_uint32)stride_width, (vx_uint32)stride_height, (vx_int32)depth_multiplier
                    };

                    operation_info.node = vxConvolutionLayer(mGraph, (vx_tensor)input.ref, (vx_tensor)mReferenceInfos[ins[1]].ref, (vx_tensor)mReferenceInfos[ins[2]].ref, (const vx_nn_convolution_params_t *)&params, sizeof(params), convTensor);

                    if (operation_info.node == nullptr)
                    {
                        LOG(ERROR) << "Create vxConvolutionLayer failed!";
                    }

                    VX_CHECK_ERROR( addActivationNode(output, convTensor, mReferenceInfos[ ins[ins.size()-1] ]) );

                }

            }
            break;
        case OperationType::LSTM:
            {
            vx_int32 index = 0;
            vx_nn_lstm_params_ext_t params;

            memset(&params, 0, sizeof(vx_nn_lstm_params_ext_t));
            vx_bool enable_layernorm = ins.size() > 23 ? vx_true_e: vx_false_e;

            index = 0;
            vx_tensor input = (vx_tensor)mReferenceInfos[ins[0]].ref;

            vx_enum precision = VX_TENSOR_PRECISION_HIGH;
            VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)mReferenceInfos[ins[12]].ref,  VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );
            VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)mReferenceInfos[ins[13]].ref,  VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );
            VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)mReferenceInfos[ins[14]].ref,  VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );
            VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)mReferenceInfos[ins[15]].ref,  VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );
            VX_CHECK_ERROR( vxSetTensorAttribute((vx_tensor)mReferenceInfos[ins[17]].ref,  VX_TENSOR_PRECISION, &precision, sizeof(vx_enum)) );

            for(auto &in : ins)
            {
                convertRankAndFormat(mGraph, mModel->operands[in], mReferenceInfos[in]);
            }

            convertScalar2Tensor(&mReferenceInfos[ins[20]]);
            convertScalar2Tensor(&mReferenceInfos[ins[21]]);
            convertScalar2Tensor(&mReferenceInfos[ins[22]]);

            /* 1 ~ 4 */
            params.base.input2input_weight = (mReferenceInfos[ins[1]].lifetime != OperandLifeTime::NO_VALUE) ? (vx_tensor)mReferenceInfos[ins[1]].ref : nullptr;
            index ++;
            params.base.input2forget_weight = (vx_tensor)mReferenceInfos[ins[2]].ref;
            params.base.input2cell_weight = (vx_tensor)mReferenceInfos[ins[3]].ref;
            params.base.input2output_weight = (vx_tensor)mReferenceInfos[ins[4]].ref;

            /* 5 ~ 8 */
            params.base.recurrent2input_weight = (vx_tensor)mReferenceInfos[ins[5]].ref;
            params.base.recurrent2forget_weight = (vx_tensor)mReferenceInfos[ins[6]].ref;
            params.base.recurrent2cell_weight = (vx_tensor)mReferenceInfos[ins[7]].ref;
            params.base.recurrent2output_weight = (vx_tensor)mReferenceInfos[ins[8]].ref;

            /* 9 ~ 11 */
            params.base.cell2input_weight = (sizeOfData(mReferenceInfos[ins[9]].type, mReferenceInfos[ins[9]].dimensions) > 0) ? (vx_tensor)mReferenceInfos[ins[9]].ref : nullptr;
            index++;
            params.base.cell2forget_weight = (sizeOfData(mReferenceInfos[ins[10]].type, mReferenceInfos[ins[10]].dimensions) > 0) ? (vx_tensor)mReferenceInfos[ins[10]].ref : nullptr;
            index++;
            params.base.cell2output_weight = (sizeOfData(mReferenceInfos[ins[11]].type, mReferenceInfos[ins[11]].dimensions) > 0) ? (vx_tensor)mReferenceInfos[ins[11]].ref : nullptr;
            index++;

            /* 12 ~ 15 */
            params.base.input_gate_bias = (sizeOfData(mReferenceInfos[ins[12]].type, mReferenceInfos[ins[12]].dimensions) > 0) ? (vx_tensor)mReferenceInfos[ins[12]].ref : nullptr;
            index++;
            params.base.forget_gate_bias = (vx_tensor)mReferenceInfos[ins[13]].ref;
            params.base.cell_bias = (vx_tensor)mReferenceInfos[ins[14]].ref;
            params.base.output_gate_bias = (vx_tensor)mReferenceInfos[ins[15]].ref;

            /* 16 ~ 17 */
            params.base.projection_weight = (sizeOfData(mReferenceInfos[ins[16]].type, mReferenceInfos[ins[16]].dimensions) > 0)  ? (vx_tensor)mReferenceInfos[ins[16]].ref : nullptr;
            index++;
            params.base.projection_bias = (sizeOfData(mReferenceInfos[ins[17]].type, mReferenceInfos[ins[17]].dimensions) > 0)  ? (vx_tensor)mReferenceInfos[ins[17]].ref : nullptr;
            index++;

            /* 18 ~ 19 */
            const VxRunTimeReferenceInfo&  output_state_in = mReferenceInfos[ins[18]];
            const VxRunTimeReferenceInfo&  cell_state_in = mReferenceInfos[ins[19]];
            /* 20 ~ 22 */
            params.base.activation = (sizeOfData(mReferenceInfos[ins[20]].type, mReferenceInfos[ins[20]].dimensions) > 0)  ? (vx_tensor)mReferenceInfos[ins[20]].ref : nullptr;
            index++;
            params.base.cell_clip = (sizeOfData(mReferenceInfos[ins[21]].type, mReferenceInfos[ins[21]].dimensions) > 0)  ? (vx_tensor)mReferenceInfos[ins[21]].ref : nullptr;
            index++;
            params.base.proj_clip = params.base.projection_weight == nullptr ? nullptr:(vx_tensor)mReferenceInfos[ins[22]].ref;

            if (enable_layernorm)
            {
                /* 23 ~ 26 */
                params.layernorm2input_weight = (vx_tensor)mReferenceInfos[ins[23]].ref;
                params.layernorm2forget_weight = (vx_tensor)mReferenceInfos[ins[24]].ref;
                params.layernorm2cell_weight = (vx_tensor)mReferenceInfos[ins[25]].ref;
                params.layernorm2output_weight = (vx_tensor)mReferenceInfos[ins[26]].ref;

            }

            if (params.base.input2input_weight == nullptr || (params.base.cell2input_weight == nullptr && params.base.cell2output_weight != nullptr))
            {
                params.base.input2input_weight = nullptr;
                params.base.recurrent2input_weight = nullptr;
                params.base.input_gate_bias = nullptr;
            }

             if (mGraph)
            {
                operation_info.node = vxLstmUnitLayer(mGraph,
                    input,
                    (vx_tensor)output_state_in.ref,
                    (vx_tensor)cell_state_in.ref,
                    (vx_nn_lstm_params_t*)&params,
                    (enable_layernorm?sizeof(vx_nn_lstm_params_ext_t):sizeof(vx_nn_lstm_params_t)),
                    (vx_tensor)mReferenceInfos[outs[0]].ref,
                    (vx_tensor)mReferenceInfos[outs[1]].ref,
                    (vx_tensor)mReferenceInfos[outs[2]].ref,
                    (vx_tensor)mReferenceInfos[outs[3]].ref);
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxLstmUnitLayer failed!";
                }

            }
            else
                LOG(ERROR) << "mGraph is NULL";

            }
            break;

        case OperationType::CONCATENATION:
            {
                vx_int32 concatAxis = getScalarData<vx_int32>(mReferenceInfos[ins[ ins.size() - 1]]);
                if(concatAxis > 4)
                {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                vx_uint32 sumConcatAxis = 0;
                for(uint32_t i = 0; i < ins.size() - 1; i++)
                {
                    sumConcatAxis += mReferenceInfos[ins[i]].dimensions[concatAxis];
                }

                if(sumConcatAxis != mReferenceInfos[outs[0]].dimensions[concatAxis])
                {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                vx_tensor *tensorVector = (vx_tensor *)malloc( sizeof(vx_tensor) * (ins.size() - 1) );
                if(tensorVector == NULL)
                {
                   LOG(ERROR) << "fail to malloc memory\n";
                }

                for(uint32_t i = 0; i < ins.size() - 1; i++)
                {
                   tensorVector[i] = (vx_tensor)mReferenceInfos[ins[i]].ref;
                }

                vx_object_array objectArray = vxCreateTensorObjectArray(vxGetContext((vx_reference)mGraph), ins.size()-1, &tensorVector[0]);

                auto convertAsixFromNHWCtoWHCN = [&](vx_uint32 TFAxis)->vx_uint32
                {
                    if (mReferenceInfos[ins[0]].dimensions.size() == 4)
                    {
                        vx_int32 nhwc2whcn[4] = { 3, 1, 0, 2 };

                        return nhwc2whcn[TFAxis];
                    }
                    else if (mReferenceInfos[ins[0]].dimensions.size() == 2)
                    {
                        vx_int32 nhwc2whcn[2] = { 1, 0 };

                        return nhwc2whcn[TFAxis];
                    }
                    else if (mReferenceInfos[ins[0]].dimensions.size() == 3)
                    {
                        vx_int32 nhwc2whcn[3] = { 2, 1, 0 };

                        return nhwc2whcn[TFAxis];
                    }

                    return mReferenceInfos[ins[0]].dimensions.size() - 1;
                };

                vx_nn_concat_params_t p = {convertAsixFromNHWCtoWHCN((vx_uint32)concatAxis)};
                operation_info.node = vxConcatIndefiniteLayer(mGraph, objectArray, &p, sizeof(p), (vx_tensor)mReferenceInfos[outs[0]].ref);

                /*vxReleaseObjectArray(&objectArray);*/
                free(tensorVector);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxConcatIndefiniteLayer failed!";
                }

                if (objectArray)
                    vxReleaseObjectArray(&objectArray);


            }
            break;
        case OperationType::DEPTH_TO_SPACE:
            {
            if (!allParametersPresent(2, 1)) {
                return ANEURALNETWORKS_BAD_DATA;
            }
            const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
            VxRunTimeReferenceInfo& blockSize = mReferenceInfos[ins[1]];

            VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];
            Shape outShape = output.shape();

            convertScalar2Tensor(&blockSize);

            vx_nn_reorg_params_t params = { (vx_tensor)blockSize.ref, VX_REORG_DEPTH_TO_SPACE };
            operation_info.node = vxReorgLayer2(mGraph, (vx_tensor)input.ref, &params, sizeof(vx_nn_reorg_params_t), (vx_tensor)output.ref);

            if (operation_info.node == nullptr)
            {
                LOG(ERROR) << "Create vxReorgLayer2 failed!";
            }

            }
            break;
        case OperationType::DEQUANTIZE:
            {
                const VxRunTimeReferenceInfo& in1 = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& out = mReferenceInfos[outs[0]];

                operation_info.node = vxTensorCopyNode(mGraph, (vx_tensor)in1.ref, (vx_tensor)out.ref);
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorCopyNode failed!";
                }
            }
            break;
        case OperationType::EMBEDDING_LOOKUP:
            {
                const VxRunTimeReferenceInfo& lookups = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& values = mReferenceInfos[ins[1]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vx_enum rank = VX_TENSOR_RANK_WHCN;
                vxSetTensorAttribute((vx_tensor)values.ref, VX_TENSOR_RANK, &rank, sizeof(vx_enum));
                vxSetTensorAttribute((vx_tensor)output.ref, VX_TENSOR_RANK, &rank, sizeof(vx_enum));

                vx_uint32 *tableDims = new vx_uint32 [4];
                vx_uint32 *outputDims = new vx_uint32 [4];

                convertDims( tableDims ,(vx_uint32 *)values.dimensions.data(), values.dimensions.size());
                convertDims( outputDims , (vx_uint32 *)output.dimensions.data(), output.dimensions.size());

                vx_tensor tableTensor = vxReshapeTensor((vx_tensor)values.ref, (vx_int32 *)tableDims, values.dimensions.size());
                vx_tensor outputTensor = vxReshapeTensor((vx_tensor)output.ref, (vx_int32 *)outputDims, output.dimensions.size());

                VX_CHECK_ERROR(vxGetStatus((vx_reference)tableTensor));
                VX_CHECK_ERROR(vxGetStatus((vx_reference)outputTensor));
                operation_info.node = vxTensorTableLookupNode2(mGraph, (vx_tensor)lookups.ref, tableTensor,outputTensor);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorTableLookupNode2 failed!";
                }

                delete [] tableDims;
                delete [] outputDims;

                vxReleaseTensor(&tableTensor);
                vxReleaseTensor(&outputTensor);


            }
            break;
        case OperationType::FLOOR:
            {
                if (!allParametersPresent(1, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vx_nn_rounding_params_t param = { VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR };
                operation_info.node = vxTensorRoundingNode(mGraph, (vx_tensor)input.ref, &param, sizeof(vx_nn_rounding_params_t), (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorRoundingNode failed!";
                }

            }
            break;
        case OperationType::FULLY_CONNECTED:
            {
                if (!allParametersPresent(4, 1)) {
                   return ANEURALNETWORKS_BAD_DATA;
                }

                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& weights = mReferenceInfos[ins[1]];
                VxRunTimeReferenceInfo& bias = mReferenceInfos[ins[2]];

                const VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vx_enum precision = VX_TENSOR_PRECISION_HIGH;
                vxSetTensorAttribute((vx_tensor)bias.ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum));

                {
#if !HIGH_PRECISION_COMPUTE
                    convertTensorFormatFromFp322Fp16(mGraph, mModel->operands[ins[1]], weights);
#endif
                    convertRankAndFormat(mGraph, mModel->operands[ins[2]], bias, true);
                }

                {
                    vx_tensor convTensor =  createTempTensor(output, mReferenceInfos[ins[3]]);

                    operation_info.node = vxFullyConnectedLayer(mGraph, (vx_tensor)input.ref, (vx_tensor)weights.ref, (vx_tensor)bias.ref,
                        0, 0,
                        VX_CONVERT_POLICY_SATURATE,
                        VX_ROUND_POLICY_TO_ZERO,
                        VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR,
                        convTensor);

                    VX_CHECK_ERROR(addActivationNode(output, convTensor, mReferenceInfos[ins[3]]) );
                }

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxFullyConnectedReluLayer failed!";
                }

            }
            break;
        case OperationType::HASHTABLE_LOOKUP:
            {
                const VxRunTimeReferenceInfo& lookups = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& keys = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& values = mReferenceInfos[ins[2]];
                VxRunTimeReferenceInfo& outputs = mReferenceInfos[outs[0]];
                VxRunTimeReferenceInfo& hits = mReferenceInfos[outs[1]];

                for(auto &in : ins)
                {
                    convertRankAndFormat(mGraph, mModel->operands[in], mReferenceInfos[in]);
                }

                vx_nn_hashlut_params_t param = { (vx_tensor)keys.ref, (vx_tensor)values.ref };
                operation_info.node = vxHashTableLookupLayer(mGraph, (vx_tensor)lookups.ref, &param, sizeof(vx_nn_hashlut_params_t), (vx_tensor)hits.ref, (vx_tensor)outputs.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxHashTableLookupLayer failed!";
                }

            }
            break;
        case OperationType::L2_NORMALIZATION:
            {
                if (!allParametersPresent(1, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                operation_info.node = vxL2NormalizeLayer(mGraph, (vx_tensor)input.ref, (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxL2NormalizeLayer failed!";
                }

            }
            break;
        case OperationType::LOCAL_RESPONSE_NORMALIZATION:
            {
                if (!allParametersPresent(5, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vx_uint32 radius = 0, norm_size = 0;
                vx_float32 bias = .0f, alpha = .0f, beta = .0f;

                vxCopyScalar((vx_scalar)mReferenceInfos[ins[1]].ref, &radius, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyScalar((vx_scalar)mReferenceInfos[ins[2]].ref, &bias, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &alpha, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &beta, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                norm_size = radius * 2 + 1;

                for(auto &in : ins)
                {
                    convertRankAndFormat(mGraph, mModel->operands[in], mReferenceInfos[in]);
                }


                vx_nn_normalization_params_t param = { VX_CONVOLUTIONAL_NETWORK_NORM_ACROSS_MAPS, norm_size, alpha, beta, bias };
                operation_info.node = vxNormalizationLayer2(mGraph, (vx_tensor)input.ref, &param, sizeof(vx_nn_normalization_params_t), (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxNormalizationLayer2 failed!";
                }

            }
            break;
        case OperationType::LSH_PROJECTION:
        {
            const VxRunTimeReferenceInfo& hash = mReferenceInfos[ins[0]];
            const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[1]];
            const VxRunTimeReferenceInfo& weight = mReferenceInfos[ins[2]];
            VxRunTimeReferenceInfo& type = mReferenceInfos[ins[3]];

            VxRunTimeReferenceInfo& outputs = mReferenceInfos[outs[0]];

            vx_enum precision = VX_TENSOR_PRECISION_HIGH;
            vxSetTensorAttribute((vx_tensor)hash.ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum));


            if (type.type == OperandType::INT32)
            {
                vx_uint32  size[] = {1}, t = 0;
                vxCopyScalar((vx_scalar)type.ref, (void*)&t, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxReleaseScalar((vx_scalar*)&type.ref);

                vx_tensor_create_params_t param0 = { 1, size, VX_TYPE_INT32, 0, {{0}}};
                type.ref = (vx_reference)vxCreateTensor2(*mContext, &param0, sizeof(vx_tensor_create_params_t));
                VX_CHECK_ERROR(vxGetStatus((vx_reference)type.ref));

                copyConstData2Tensor( (vx_tensor & )type.ref, (vx_uint8 *)&t, VX_WRITE_ONLY);
            }

            convertRankAndFormat(mGraph, mModel->operands[ins[0]], mReferenceInfos[ins[0]]);

            vx_nn_lshproj_params_t param = { (vx_tensor)hash.ref, (vx_tensor)weight.ref, (vx_tensor)type.ref };
            operation_info.node = vxLSHProjectionLayer(mGraph, (vx_tensor)input.ref, &param, sizeof(vx_nn_lshproj_params_t), (vx_tensor)outputs.ref);

            if (operation_info.node == nullptr)
            {
                LOG(ERROR) << "Create vxLSHProjectionLayer failed!";
            }

            if (type.type == OperandType::INT32 && type.ref)
                vxReleaseTensor((vx_tensor*)&type.ref);

        }
            break;
        case OperationType::MAX_POOL_2D:
        case OperationType::AVERAGE_POOL_2D:
        case OperationType::L2_POOL_2D:
            {
                const size_t inCount = ins.size();
                if ((inCount != 10 && inCount != 7) ||
                    !allParametersPresent(inCount, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                };

                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];

                vx_int32 padding_left, padding_right;
                vx_int32 padding_top, padding_bottom;
                vx_uint32 stride_width, stride_height;
                vx_uint32 filter_width, filter_height;
                vx_uint32 activation;

                if (inCount == 10) {
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[1]].ref, &padding_left, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[2]].ref, &padding_right, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &padding_top, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &padding_bottom, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[5]].ref, &stride_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[6]].ref, &stride_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[7]].ref, &filter_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[8]].ref, &filter_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[9]].ref, &activation, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                }
                else {
                    vx_uint32 padding_implicit = 0;
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[1]].ref, &padding_implicit, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[2]].ref, &stride_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[3]].ref, &stride_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[4]].ref, &filter_width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[5]].ref, &filter_height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyScalar((vx_scalar)mReferenceInfos[ins[6]].ref, &activation, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                    Shape inputShape = input.shape();
                    vx_uint32 input_width = getSizeOfDimension(inputShape, 2);
                    vx_uint32 input_height = getSizeOfDimension(inputShape, 1);
                    calculateExplicitPadding(input_width, stride_width,
                        filter_width, padding_implicit,
                        &padding_left, &padding_right);
                    calculateExplicitPadding(input_height, stride_height,
                        filter_height, padding_implicit,
                        &padding_top, &padding_bottom);

                }

                const VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                /*TODO: check the data format*/
                vx_tensor o = (vx_tensor)output.ref;
                vx_enum data_format =  OperandType::TENSOR_QUANT8_ASYMM == output.type ? VX_TYPE_UINT8 : VX_TYPE_FLOAT16;
                vx_tensor_create_params_t p = { (vx_uint32)output.dimensions.size(), (vx_uint32*)output.dimensions.data(), data_format, 0, {{0}} };

                vx_nn_pooling_params_t params = { VX_CONVOLUTIONAL_NETWORK_POOLING_MAX, filter_width, filter_height, (vx_uint32)padding_left, (vx_uint32)padding_right,
                    (vx_uint32)padding_top, (vx_uint32)padding_bottom, /*stride_width, stride_height,*/ VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR };

                if (OperationType::AVERAGE_POOL_2D == operation.type)
                {
                    params.pool_type = VX_CONVOLUTIONAL_NETWORK_POOLING_AVG_ANDROID;
                }
                else if (OperationType::L2_POOL_2D == operation.type)
                {
                    params.pool_type = VX_CONVOLUTIONAL_NETWORK_POOLING_L2;
                }

                if (activation > 0)
                {
                    o = vxCreateVirtualTensor2(mGraph, &p, sizeof(vx_tensor_create_params_t));
                    {
                        vx_enum rank = VX_TENSOR_RANK_CWHN;
                        vxSetTensorAttribute((vx_tensor)o, VX_TENSOR_RANK, &rank, sizeof(vx_enum));
                    }
                }

                operation_info.node = vxPoolingLayer2(mGraph,
                    (vx_tensor)input.ref,
                    &params,
                    sizeof(vx_nn_pooling_params_t),
                    o);

                if (FusedActivationFunctionType::kNone != (FusedActivationFunctionType)activation)
                {
                    vxActivationLayer(mGraph, o, getOvxActivationType((FusedActivationFunctionType)activation), 0, 0, (vx_tensor)output.ref);
                }

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxPoolingLayer2 failed!";
                }

                if (activation > 0 && o)
                    vxReleaseTensor(&o);

            }
            break;
        case OperationType::RELU:
        case OperationType::RELU1:
        case OperationType::RELU6:
        case OperationType::LOGISTIC:
        case OperationType::TANH:
            {
                if (!allParametersPresent(1, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vx_enum type = 0;
                vx_int32 a = 0, b = 0;

                if (operation.type == OperationType::RELU)
                    type = VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU;
                else if (operation.type == OperationType::RELU1)
                    type = VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU1;
                else if (operation.type == OperationType::RELU6)
                    type = VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU6;
                else if (operation.type == OperationType::LOGISTIC)
                    type = VX_CONVOLUTIONAL_NETWORK_ACTIVATION_LOGISTIC;
                else if (operation.type == OperationType::TANH)
                {
                    type = VX_CONVOLUTIONAL_NETWORK_ACTIVATION_HYPERBOLIC_TAN;
                    a = 1;
                    b = 1;
                }

                operation_info.node = vxActivationLayer(mGraph, (vx_tensor)input.ref, type, a, b, (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxActivationLayer failed!";
                }

            }
            break;
            break;
        case OperationType::RESHAPE:
        {
            if (!allParametersPresent(2, 1)) {
                return ANEURALNETWORKS_BAD_DATA;
            }
            const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
            const VxRunTimeReferenceInfo& targetShape = mReferenceInfos[ins[1]];
            VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

            /*viv:
                in driver stream, the tensor is whcn,
                so that it need to be converted to be cwhn ,flattened and coverted to whcn.
            */
            vx_uint32 perm_whcn2cwhn[] = {2, 0, 1, 3};
            vx_uint32 perm_cwhn2whcn[] = {1, 2, 0, 3};
            vx_tensor tfTensorIn = (vx_tensor)input.ref;
            vx_tensor tfTensorOut =(vx_tensor)output.ref;
            if(input.dimensions.size() == 4 && input.dimensions[3] > 1 && input.dimensions[1] * input.dimensions[2] > 1)
            {
                vx_uint32 cwhnFromNhwc[] = {input.dimensions[3], input.dimensions[2],input.dimensions[1],input.dimensions[0]};
                tfTensorIn = creatVirtualTensorByParam(mGraph, 4,cwhnFromNhwc, input.type, input.scale, input.zeroPoint);
                vx_node tfNodeIn = vxTensorPermuteNode(mGraph, (vx_tensor)input.ref, tfTensorIn, perm_whcn2cwhn, 4);
                vxReleaseNode(&tfNodeIn);
            }

            if(output.dimensions.size() == 4 && output.dimensions[3] > 1 && output.dimensions[1] * output.dimensions[2] > 1){
                vx_uint32 whcnFromNhwc[] = {output.dimensions[2], output.dimensions[1], output.dimensions[3], output.dimensions[0]};
                tfTensorOut = creatVirtualTensorByParam(mGraph, 4, whcnFromNhwc, output.type, output.scale, output.zeroPoint);
            }

            vx_nn_reshape_params_t param = { (vx_tensor)targetShape.ref};
            operation_info.node = vxTensorReshapeNode(mGraph, tfTensorIn, &param, sizeof(vx_nn_reshape_params_t), tfTensorOut);

            if(output.dimensions.size() == 4 && output.dimensions[3] > 1 && output.dimensions[1] * output.dimensions[2] > 1){
                vx_node tfNodeOut = vxTensorPermuteNode(mGraph, tfTensorOut, (vx_tensor)output.ref, perm_cwhn2whcn, 4);

                if (tfTensorOut)
                    vxReleaseTensor(&tfTensorOut);

                vxReleaseNode(&tfNodeOut);
            }

            if (operation_info.node == nullptr)
            {
                LOG(ERROR) << "Create vxTensorReshapeNode failed!";
            }


        }

            break;
        case OperationType::RESIZE_BILINEAR:
            {
                if (!allParametersPresent(3, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                vx_int32 width = 0, height = 0;
                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vxCopyScalar((vx_scalar)mReferenceInfos[ins[1]].ref, &width, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyScalar((vx_scalar)mReferenceInfos[ins[2]].ref, &height, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                vx_nn_scale_params_t param = { VX_INTERPOLATION_BILINEAR };
                operation_info.node = vxTensorScaleNode(mGraph, (vx_tensor)input.ref, &param, sizeof(vx_nn_scale_params_t), (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorScaleNode failed!";
                }

            }
            break;
        case OperationType::RNN:
            {
                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& weight = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& recurrent_weight = mReferenceInfos[ins[2]];
                const VxRunTimeReferenceInfo& bias = mReferenceInfos[ins[3]];
                const VxRunTimeReferenceInfo& state_in = mReferenceInfos[ins[4]];
                VxRunTimeReferenceInfo& activation = mReferenceInfos[ins[5]];

                const VxRunTimeReferenceInfo& state_out = mReferenceInfos[outs[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[1]];

                for(auto &in : ins)
                {
                    convertRankAndFormat(mGraph, mModel->operands[in], mReferenceInfos[in]);
                }

                convertScalar2Tensor(&activation);

                vx_nn_rnn_params_t param = { (vx_tensor)weight.ref, (vx_tensor)recurrent_weight.ref, (vx_tensor)bias.ref, (vx_tensor)state_in.ref,  (vx_tensor)activation.ref };
                operation_info.node = vxRNNLayer(mGraph, (vx_tensor)input.ref, &param, sizeof(vx_nn_rnn_params_t), (vx_tensor)state_out.ref, (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxRNNLayer failed!";
                }

            }
            break;
        case OperationType::SOFTMAX:
            {
                if (!allParametersPresent(2, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                vx_float32 beta = .0f;
                const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];

                vxCopyScalar((vx_scalar)mReferenceInfos[ins[1]].ref, &beta, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                vx_nn_softmax_params_t param = { beta };
                operation_info.node = vxSoftmaxLayer2(mGraph, (vx_tensor)input.ref, &param, sizeof(vx_nn_softmax_params_t), (vx_tensor)output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxSoftmaxLayer2 failed!";
                }

            }
            break;
        case OperationType::SPACE_TO_DEPTH:
        {
            if (!allParametersPresent(2, 1)) {
                return ANEURALNETWORKS_BAD_DATA;
            }
            const VxRunTimeReferenceInfo& input = mReferenceInfos[ins[0]];
            VxRunTimeReferenceInfo& blockSize = mReferenceInfos[ins[1]];

            VxRunTimeReferenceInfo& output = mReferenceInfos[outs[0]];
            Shape outShape = output.shape();

            convertScalar2Tensor(&blockSize);

            vx_nn_reorg_params_t params = { (vx_tensor)blockSize.ref, VX_REORG_SPACE_TO_DEPTH };

            operation_info.node = vxReorgLayer2(mGraph, (vx_tensor)input.ref, &params, sizeof(vx_nn_reorg_params_t), (vx_tensor)output.ref);

            if (operation_info.node == nullptr)
            {
                LOG(ERROR) << "Create vxReorgLayer2 failed!";
            }

        }
            break;
        case OperationType::SVDF:
            {
                for(auto &in : ins)
                {
                    convertRankAndFormat(mGraph, mModel->operands[in], mReferenceInfos[in]);
                }
                const VxRunTimeReferenceInfo& input           = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& weight_feature  = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& weight_time     = mReferenceInfos[ins[2]];
                const VxRunTimeReferenceInfo& bias            = mReferenceInfos[ins[3]];
                const VxRunTimeReferenceInfo& state_in        = mReferenceInfos[ins[4]];
                VxRunTimeReferenceInfo& ranks           = mReferenceInfos[ins[5]];
                VxRunTimeReferenceInfo& activation      = mReferenceInfos[ins[6]];

                const VxRunTimeReferenceInfo& state_out = mReferenceInfos[outs[0]];
                VxRunTimeReferenceInfo& output = mReferenceInfos[outs[1]];

                convertScalar2Tensor(&ranks);
                convertScalar2Tensor(&activation);

                vx_enum precision = VX_TENSOR_PRECISION_HIGH;
                vxSetTensorAttribute((vx_tensor)bias.ref, VX_TENSOR_PRECISION, &precision, sizeof(vx_enum));

                /*vx_nn_svdf_params_t param = { (vx_tensor)weight_feature.ref, (vx_tensor)weight_time.ref, (vx_tensor)bias.ref, (vx_tensor)state_in.ref, (vx_tensor)ranks.ref, (vx_tensor)activation.ref };*/
                vx_nn_svdf_params_t p;
                p.weights_feature = (vx_tensor)weight_feature.ref;
                p.recurrent_time = (vx_tensor)weight_time.ref;
                p.bias = (vx_tensor)bias.ref;
                p.state_in = (vx_tensor)state_in.ref;
                p.rank = (vx_tensor)ranks.ref;
                p.activation = (vx_tensor)activation.ref;
                operation_info.node = vxSVDFLayer(mGraph, (vx_tensor)input.ref, &p, sizeof(vx_nn_svdf_params_t), (vx_tensor) state_out.ref, (vx_tensor) output.ref);

                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxSVDFLayer failed!";
                }

            }
            break;

#if ANDROID_SDK_VERSION > 27

        case OperationType::DIV:{
                if (!allParametersPresent(3, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0  = mReferenceInfos[ins[0]];
                VxRunTimeReferenceInfo& input1  = mReferenceInfos[ins[1]];
                VxRunTimeReferenceInfo& activation     = mReferenceInfos[ins[2]];
                VxRunTimeReferenceInfo& output          = mReferenceInfos[outs[0]];

                if (input1.ref == nullptr)
                {
                    input1.ref = (vx_reference)creatTensorByParam(vxGetContext((vx_reference)mGraph), input0.dimensions.size(), (vx_uint32 *)input0.dimensions.data(), input0.type, input0.scale, input0.zeroPoint);
                }

                vx_float32 scaler = 1.0f;
                vx_scalar vxScaler = vxCreateScalar(vxGetContext((vx_reference)mGraph), VX_TYPE_FLOAT32, &scaler);

                vx_tensor tmpTensor = createTempTensor(output, activation);

                operation_info.node = vxTensorDivideNode(mGraph, (vx_tensor)input0.ref, (vx_tensor)input1.ref,  vxScaler, VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, tmpTensor );
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorDivideNode failed!";
                }
                VX_CHECK_ERROR( addActivationNode(output, tmpTensor, activation) );

                VX_CHECK_ERROR( vxReleaseScalar(&vxScaler) );

                break;
            }
        case OperationType::SUB:{
                if (!allParametersPresent(3, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }

                const VxRunTimeReferenceInfo& input0  = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& input1  = mReferenceInfos[ins[1]];
                VxRunTimeReferenceInfo& activation     = mReferenceInfos[ins[2]];
                VxRunTimeReferenceInfo& output          = mReferenceInfos[outs[0]];

                vx_tensor tmpTensor = createTempTensor(output, activation);
                operation_info.node = vxTensorSubtractNode(mGraph, (vx_tensor)input0.ref, (vx_tensor)input1.ref, VX_CONVERT_POLICY_WRAP, tmpTensor);
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorSubtractNode failed!";
                }
                VX_CHECK_ERROR( addActivationNode( output, tmpTensor, activation) );
            break;
        }
        case OperationType::TRANSPOSE:{
                if (!allParametersPresent(2, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0  = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& perm  = mReferenceInfos[ins[1]];
                VxRunTimeReferenceInfo& output          = mReferenceInfos[outs[0]];

                vx_uint32 permuteIdx[4];
                for(vx_uint32 i = 0; i < input0.dimensions.size(); i++)
                    permuteIdx[i] = input0.dimensions[input0.dimensions.size() - 1 - i];
               copyConstData2Tensor((vx_tensor &)perm.ref, (vx_uint8 *)permuteIdx, VX_READ_ONLY);

               vx_uint32 ovxRank[4] = {0};
                if(input0.dimensions.size() == 4)
                {
                    vx_uint32 order[] = { 2, 1, 3, 0 };
                    vx_uint32 whcn_order[] = { 3, 1, 0, 2 };
                    vx_uint32 whcn_out[] = { permuteIdx[order[0]], permuteIdx[order[1]], permuteIdx[order[2]], permuteIdx[order[3]] };
                    for(int i = 0; i < 4; i ++)
                        ovxRank[i] = whcn_order[whcn_out[i] ];
                }
                else if(input0.dimensions.size() == 3)
                {
                    vx_uint32 order[] = {2, 0, 1};
                    for(int i = 0; i < 3; i++)
                        ovxRank[i] = permuteIdx[ order[i] ];
                }
                else if(input0.dimensions.size() == 2)
                {
                    vx_uint32 order[] = {0,1};
                    for(int i = 0; i < 2; i++)
                        ovxRank[i] = permuteIdx[order[i]];
                }

                operation_info.node = vxTensorPermuteNode(mGraph, RUNTIME_TENSOR(input0), RUNTIME_TENSOR( output), ovxRank, input0.dimensions.size());
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorSubtractNode failed!";
                }
            break;
        }
        case OperationType::MEAN:  {
                if (!allParametersPresent(3, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0  = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& input1  = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& input2     = mReferenceInfos[ins[2]];
                VxRunTimeReferenceInfo& output          = mReferenceInfos[outs[0]];

                vx_int32 scalar = 0;
                VX_CHECK_ERROR(vxCopyScalar((vx_scalar)input2.ref, &scalar, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
                vx_nn_mean_params_t params = { (vx_tensor)input1.ref, scalar};

                vx_int32 axisDims_org[4] = {0};
                vx_int32 axisDims[4] = { 0 };
                vx_int32 converts[][4] = {
                    { 0 },
                    { 1, 0 },
                    { 2, 1, 0 },
                    { 3, 1, 0, 2},
                    };

                copyConstData2Tensor((vx_tensor &)input1.ref, (vx_uint8*)axisDims_org, VX_READ_ONLY);

                for (vx_uint32 i = 0; i < input1.dimensions[0]; i ++)    {
                    vx_int32 index = (axisDims_org[i] < 0) ? (axisDims_org[i] + input0.dimensions.size()) : axisDims_org[i];
                    axisDims[i] = converts[ input0.dimensions.size() - 1][index];
                    }

                copyConstData2Tensor((vx_tensor &)input1.ref, (vx_uint8*)axisDims, VX_WRITE_ONLY);

                operation_info.node = vxTensorMeanNode(mGraph, (vx_tensor)input0.ref, &params, sizeof(vx_nn_mean_params_t), (vx_tensor)output.ref);
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorMeanNode failed!";
                }
            break;
        }
        case OperationType::SQUEEZE:{
                if (!allParametersPresent(2, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0  = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& input1  = mReferenceInfos[ins[1]];
                VxRunTimeReferenceInfo& output          = mReferenceInfos[outs[0]];

                vx_nn_squeeze_params_t params = { (vx_tensor)input1.ref};
                operation_info.node = vxTensorSqueezeNode(mGraph, (vx_tensor)input0.ref, &params, sizeof(vx_nn_squeeze_params_t), (vx_tensor)output.ref);
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorMeanNode failed!";
                }
            break;
        }
        case OperationType::STRIDED_SLICE:{
                if (!allParametersPresent(7, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0  = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& input1  = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& input2  = mReferenceInfos[ins[2]];
                const VxRunTimeReferenceInfo& input3  = mReferenceInfos[ins[3]];
                const VxRunTimeReferenceInfo& input4  = mReferenceInfos[ins[4]];
                const VxRunTimeReferenceInfo& input5  = mReferenceInfos[ins[5]];
                const VxRunTimeReferenceInfo& input6  = mReferenceInfos[ins[6]];

                VxRunTimeReferenceInfo& output        = mReferenceInfos[outs[0]];

                convertTensor(input1, input0.dimensions.size());
                convertTensor(input2, input0.dimensions.size());
                convertTensor(input3, input0.dimensions.size());

                vx_int32 scalar4 = convertScalar(input4, input0.dimensions.size());
                vx_int32 scalar5 = convertScalar(input5, input0.dimensions.size());
                vx_int32 scalar6 = convertScalar(input6, input0.dimensions.size());

                vx_nn_stride_slice_params_t params = {
                        RUNTIME_TENSOR(input1),
                        RUNTIME_TENSOR(input2),
                        RUNTIME_TENSOR(input3),
                        scalar4,
                        scalar5,
                        scalar6};
                operation_info.node = vxTensorStrideSliceNode(mGraph, RUNTIME_TENSOR(input0), &params, sizeof(vx_nn_stride_slice_params_t), RUNTIME_TENSOR(output));
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxTensorStrideSliceNode failed!";
                }
            break;
        }
        case OperationType::SPACE_TO_BATCH_ND:{
                if (!allParametersPresent(3, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0      = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& block_sizes = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& paddings    = mReferenceInfos[ins[2]];
                const VxRunTimeReferenceInfo& output0     = mReferenceInfos[outs[0]];

                auto Convert_tensor_for_space2batch = [](int dimCount, vx_tensor tensor)->void
                {
                    vx_int32 tensor_value[4] = { 0 }, temp = 0;

                    copyConstData2Tensor(tensor, (vx_uint8 *)tensor_value, VX_READ_ONLY);
                    if (dimCount == 1)
                    {
                        temp = tensor_value[0];
                        tensor_value[0] = tensor_value[1];
                        tensor_value[1] = temp;
                    }
                    else if (dimCount == 2)
                    {
                        temp = tensor_value[0];
                        tensor_value[0] = tensor_value[2];
                        tensor_value[2] = temp;
                        temp = tensor_value[1];
                        tensor_value[1] = tensor_value[3];
                        tensor_value[3] = temp;
                    }
                    copyConstData2Tensor(tensor, (vx_uint8 *)tensor_value, VX_WRITE_ONLY);
                };

                Convert_tensor_for_space2batch(block_sizes.dimensions.size(), (vx_tensor)block_sizes.ref);
                Convert_tensor_for_space2batch(paddings.dimensions.size(),  (vx_tensor)paddings.ref);

                vx_nn_reorg_params_ext_t p = { { RUNTIME_TENSOR(block_sizes), VX_REORG_SPACE_TO_BATCH_ND }, RUNTIME_TENSOR(paddings)};
                operation_info.node = vxReorgLayer2(mGraph, RUNTIME_TENSOR(input0), (vx_nn_reorg_params)&p, sizeof(p), RUNTIME_TENSOR(output0));
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxReorgLayer2 failed!";
                }
            break;
        }
        case OperationType::BATCH_TO_SPACE_ND: {
                if (!allParametersPresent(2, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0      = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& block_sizes = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& output0     = mReferenceInfos[outs[0]];

                vx_int32 block_size[4] = { 0 },temp = 0;
                copyConstData2Tensor((vx_tensor &)block_sizes.ref, (vx_uint8 *)block_size, VX_READ_ONLY);
                temp = block_size[0];
                block_size[0] = block_size[1];
                block_size[1] = temp;
                copyConstData2Tensor((vx_tensor &)block_sizes.ref, (vx_uint8 *)block_size, VX_WRITE_ONLY);

                vx_nn_reorg_params_t  p = { RUNTIME_TENSOR(block_sizes), VX_REORG_BATCH_TO_SPACE_ND };
                operation_info.node = vxReorgLayer2(mGraph, RUNTIME_TENSOR(input0), (vx_nn_reorg_params)&p, sizeof(p), RUNTIME_TENSOR(output0));
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxReorgLayer2 failed!";
                }
            break;
        }
        case OperationType::PAD:  {
                if (!allParametersPresent(2, 1)) {
                    return ANEURALNETWORKS_BAD_DATA;
                }
                const VxRunTimeReferenceInfo& input0      = mReferenceInfos[ins[0]];
                const VxRunTimeReferenceInfo& padding   = mReferenceInfos[ins[1]];
                const VxRunTimeReferenceInfo& output0     = mReferenceInfos[outs[0]];

                int32_t pad_fronts[4] = { 0 }, pad_backs[4] = { 0 }, pads[8] = {0};
                copyConstData2Tensor((vx_tensor &)padding.ref, (vx_uint8 *) pads, VX_READ_ONLY);

                /*WHCN: W  */
                pad_fronts[0] = pads[4];
                pad_backs[0]  = pads[5];

                /*WHCN: H  */
                pad_fronts[1] = pads[2];
                pad_backs[1]  = pads[3];

                /*WHCN: C  */
                pad_fronts[2] = pads[6];
                pad_backs[2]  = pads[7];

                /*WHCN: N  */
                pad_fronts[3] = pads[0];
                pad_backs[3]  = pads[1];

                vx_nn_pad_params_t p = { pad_fronts, pad_backs, (vx_uint8)padding.dimensions[0], VX_PAD_CONSTANT, NULL };

                operation_info.node = vxTensorPadNode(mGraph, RUNTIME_TENSOR(input0), RUNTIME_TENSOR(output0), &p, sizeof(p));
                if (operation_info.node == nullptr)
                {
                    LOG(ERROR) << "Create vxReorgLayer2 failed!";
                }
            break;
        }
#endif
        case OperationType::OEM_OPERATION:
            break;
        }

        mOperationInfos.push_back(operation_info);

        status = vxGetStatus((vx_reference)operation_info.node);

        if (status != VX_SUCCESS)
            break;
    }

    return status;
}

int OvxExecutor::initalize(vx_context* context, pthread_mutex_t* mutex, const Model* model, std::vector<VxRunTimePoolInfo>* poolInfos)
{

    if (mModel == nullptr)
        mModel = model;
    else
        LOG(ERROR) << "mModel is not null!";

    setRunTimePoolInfosFromHidlMemories(poolInfos, mModel->pools);

    mMutex = mutex;

    pthread_mutex_lock(mMutex);

    mContext = context;

    *mContext = vxCreateContext();

    pthread_mutex_unlock(mMutex);

    getGraph(poolInfos);

    if(vxVerifyGraph(mGraph) != VX_SUCCESS)
    {
        LOG(ERROR)<<"verify graph fail";
        nnAssert(0);
    }

   /* initalizeEnv();*/
    return ANEURALNETWORKS_NO_ERROR;
}

void syncIfNeed(Model model, std::vector<VxRunTimeReferenceInfo> infos)
{

    const vx_int32 operand_count =  model.outputIndexes.size(); /* model.operations.size();*/
    LOG(ERROR) << "sync output Count " << operand_count;
    for (vx_int32 i = 0; i < operand_count; i++)
    {
        VxRunTimeReferenceInfo& info = infos[model.outputIndexes[i]];

        vx_int32 item_size = (info.type == OperandType::TENSOR_QUANT8_ASYMM)?sizeof(vx_uint8):sizeof(vx_float32);
        vx_int32 count = sizeOfData(info.type, info.dimensions)/item_size;

        /*Maybe we can not get tensor's internal memory address*/
        if(info.buffer)
            memcpy(info.original, info.buffer, count * item_size);
        else
            copyConstData2Tensor( (vx_tensor &)info.ref , info.original, VX_READ_ONLY);

    }
}

/* Ignore the .pools entry in model and request.  This will have been taken care of by the caller. */
int OvxExecutor::run(const Model& model, const Request& request,
    const std::vector<VxRunTimePoolInfo>& requestPoolInfos) {

    mModel = &model;
    mRequest = &request;

    double t0 = getTime();

    initializeRunTimeInfo(requestPoolInfos);

    double t1 = getTime();

    vxProcessGraph(mGraph);

    double t2 = getTime();

    syncIfNeed(model, mReferenceInfos);

    double t3 = getTime();

    if(preformaceDebug){
        LOG(INFO)<<"time of copying  host to device: "<<(t1 - t0) * 1000<<"ms\n";
        LOG(INFO)<<"time of processing graph: "<<(t2 - t1) * 1000<<"ms\n";
        LOG(INFO)<<"time of copying  device to host: "<<(t3 - t2) * 1000<<"ms\n";
        }

    if (*mContext != nullptr && *mContext != mPreContext)
    {
        mPreContext = *mContext;
    }

    return ANEURALNETWORKS_NO_ERROR;
}

void OvxExecutor::initalizeEnv()
{
    char *env =  getenv("GET_PREF");
    LOG(ERROR)<<"get env for pref:"<<env<<"\n";
    if(env)
        preformaceDebug = vx_true_e;

}

bool OvxExecutor::initializeRunTimeInfo(const std::vector<VxRunTimePoolInfo>& requestPoolInfos) {
    const size_t count = mModel->operands.size();

    if (mReferenceInfos.size() != count)
        mReferenceInfos.resize(count);

    // Adjust the runtime info for the arguments passed to the model,
    // modifying the buffer location, and possibly the dimensions.
    auto updateForArguments = [this, &requestPoolInfos](const std::vector<uint32_t>& indexes,
        const hidl_vec<RequestArgument>& arguments) {
        nnAssert(indexes.size() == arguments.size());
        for (size_t i = 0; i < indexes.size(); i++) {
            const uint32_t operandIndex = indexes[i];
            const RequestArgument& from = arguments[i];
            VxRunTimeReferenceInfo& to = mReferenceInfos[operandIndex];
            if (from.dimensions.size() > 0) {
                // It's the responsibility of the caller to validate that
                // from.dimensions only modifies the dimensions that were
                // unspecified in the model.  That's the case in SampleDriver.cpp
                // with the call to validateRequest().
                // TODO make sure that's the case for the default CPU path.
                to.dimensions = from.dimensions;
            }
            if (from.hasNoValue) {
                to.lifetime = OperandLifeTime::NO_VALUE;
                nnAssert(to.buffer == nullptr);

                vx_bool valued = vx_false_e;
                vx_enum lifetime = VX_TENSOR_LIFE_TIME_DYNAMIC;
                vxSetTensorAttribute((vx_tensor)to.ref, VX_TENSOR_VALUE, &valued, sizeof(vx_bool));
                vxSetTensorAttribute((vx_tensor)to.ref, VX_TENSOR_LIFETIME, &lifetime, sizeof(vx_enum));
            }
            else {
                auto poolIndex = from.location.poolIndex;
                nnAssert(poolIndex < requestPoolInfos.size());
                auto& r = requestPoolInfos[poolIndex];

                //if (to.original == nullptr)
                    to.original = r.buffer + from.location.offset;

                if (to.buffer == nullptr)
                    VX_CHECK_ERROR( vxSwapTensorHandle((vx_tensor)to.ref, NULL, (void **)&to.buffer) );

                if (to.lifetime == OperandLifeTime::MODEL_INPUT)
                {
                    copyConstData2Tensor( (vx_tensor &)to.ref , to.original, VX_WRITE_ONLY);
                }
            }
        }
    };
    updateForArguments(mModel->inputIndexes, mRequest->inputs);
    updateForArguments(mModel->outputIndexes, mRequest->outputs);

    return true;
}

bool OvxExecutor::deinitializeRunTimeInfo() {

    pthread_mutex_lock(mMutex);

    for (VxOperationInfo info : mOperationInfos)
    {
        if (info.node)vxReleaseNode((vx_node*)&info.node);

        for (VxReferenceInfo ref : info.refs)
        {
            switch (ref.type)
            {
            case VX_TYPE_SCALAR:
                vxReleaseScalar((vx_scalar*)&ref.ref);
                break;
            case VX_TYPE_WEIGHTS_BIASES_PARAMETER:
                vxReleaseWeightsBiasesParameter((vx_weights_biases_parameter*)&ref.ref);
                break;
            default:
                LOG(ERROR) << "Cannot release the reference which type is " << ref.type << ", Not support!";
                break;
            }
        }
    }

    for (VxRunTimePoolInfo pool : mRequestPoolInfos)
    {
        if (pool.ref)
        {
            vxReleaseTensor((vx_tensor*)&pool.ref);
            pool.ref = nullptr;
        }
    }

    for (VxRunTimeReferenceInfo info : mReferenceInfos)
    {
        if (info.ref)
        {
            vxReleaseTensor((vx_tensor*)&info.ref);
            info.ref = nullptr;
        }
    }

    if (mGraph && (mPreContext == vxGetContext((vx_reference)mGraph)))
    {
        vxReleaseGraph(&mGraph);
        mGraph = nullptr;
    }

#ifdef MULTI_CONTEXT
    if (mPreContext)
    {
        vxReleaseContext(&mPreContext);
        mPreContext = nullptr;
    }
#endif

    pthread_mutex_unlock(mMutex);


    return true;
}

void VxRunTimePoolInfo::release() {
    if (buffer == nullptr) {
        return;
    }

    auto memType = hidlMemory.name();
    if (memType == "ashmem") {
        // nothing to do
    } else if (memType == "mmap_fd") {
        size_t size = hidlMemory.size();
        if (munmap(buffer, size)) {
            LOG(ERROR) << "VxRunTimePoolInfo::release(): Can't munmap";
        }
    } else if (memType == "") {
        // Represents a POINTER argument; nothing to do
    } else {
        LOG(ERROR) << "VxRunTimePoolInfo::release(): unsupported hidl_memory type";
    }

    hidlMemory = hidl_memory();
    memory     = nullptr;
    buffer     = nullptr;
}

} /* namespace nn */
} /* namespace android */

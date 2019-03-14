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


#ifndef ANDROID_ML_NN_COMMON_OVX_EXECUTOR_H
#define ANDROID_ML_NN_COMMON_OVX_EXECUTOR_H

#include "HalInterfaces.h"
#include "OperationsUtils.h"
#include "Utils.h"


#include <algorithm>
#include <vector>

#include <VX/vx.h>
#include <VX/vx_api.h>
#include <VX/vx_khr_cnn.h>
#ifdef VX_VERSION_1_2
#include <VX/viv_nn_compatibility.h>
#endif
#define VX_CHECK_ERROR(status) {\
    if(VX_SUCCESS != status){\
        LOG(ERROR)<<"status error"<< status<< " line:"<<__LINE__;\
        }\
}

#define RUNTIME_TENSOR(runtimeInfo) ((vx_tensor)( (runtimeInfo).ref))

namespace android {
namespace nn {

enum class FusedActivationFunctionType { kNone = 0,  kRelu , kRelu1, kRelu6, };
// Used to keep a pointer to each of the memory pools.
//
// In the case of an "mmap_fd" pool, owns the mmap region
// returned by getBuffer() -- i.e., that region goes away
// when the VxRunTimePoolInfo is destroyed or is assigned to.
class VxRunTimePoolInfo {
public:
    // If "fail" is not nullptr, and construction fails, then set *fail = true.
    // If construction succeeds, leave *fail unchanged.
    // getBuffer() == nullptr IFF construction fails.
    explicit VxRunTimePoolInfo(const hidl_memory& hidlMemory, bool* fail);

    explicit VxRunTimePoolInfo(uint8_t* buffer);
    ~VxRunTimePoolInfo() { release(); }

    uint8_t* buffer = nullptr;  // always used

    vx_reference ref = nullptr;

private:

    bool set(const hidl_memory& hidlMemory);
    bool update();

    hidl_memory hidlMemory;     // always used
    sp<IMemory> memory;         // only used when hidlMemory.name() == "ashmem"

    vx_uint32 size = 0;


    std::vector<vx_uint8> values;

    void release();
    void moveFrom(VxRunTimePoolInfo&& other);

};

bool setRunTimePoolInfosFromHidlMemories(std::vector<VxRunTimePoolInfo>* poolInfos,
                                         const hidl_vec<hidl_memory>& pools);

// Information we maintain about each operand during execution that
// may change during execution.
struct VxRunTimeReferenceInfo {
    // TODO Storing the type here is redundant, as it won't change during execution.
    OperandType type;

    // The type and dimensions of the operand.  The dimensions can
    // change at runtime.  We include the type because it's useful
    // to pass together with the dimension to the functions implementing
    // the operators.
    std::vector<uint32_t> dimensions;

    vx_float32 scale;
    int32_t zeroPoint;
    // Where the operand's data is stored.  Check the corresponding
    // location information in the model to figure out if this points
    // to memory we have allocated for an temporary operand.
    uint8_t* buffer = nullptr;
    uint8_t* original = nullptr;
    // The length of the buffer.
    uint32_t length;

    vx_reference ref = nullptr;
    // Whether this is a temporary variable, a model input, a constant, etc.
    OperandLifeTime lifetime;
    // Keeps track of how many operations have yet to make use
    // of this temporary variable.  When the count is decremented to 0,
    // we free the buffer.  For non-temporary variables, this count is
    // always 0.
    uint32_t numberOfUsesLeft;

    void* pre_buffer = nullptr;

    Shape shape() const {
        return Shape{ .type = type, .dimensions = dimensions, .scale = scale, .offset = zeroPoint };
    }
};
struct VxReferenceInfo
{
    vx_reference ref;
    vx_enum type;
};

struct VxOperationInfo
{
    Operation operation;

    vx_node node;
    std::vector<VxReferenceInfo> refs;
};

/* This class is used to execute a model with ovx implement. */
class OvxExecutor : public RefBase{
public:
    OvxExecutor() {}

    ~OvxExecutor() {}

    int run(const Model& model, const Request& request,
        const std::vector<VxRunTimePoolInfo>& requestPoolInfos);

    int initalize(vx_context context, const Model* model, std::vector<VxRunTimePoolInfo>* poolInfos);

    bool deinitializeRunTimeInfo();

    std::vector<VxRunTimePoolInfo> mRequestPoolInfos;

    std::vector<VxRunTimeReferenceInfo> mReferenceInfos;

private:

    vx_uint8* getPointer(Operand operand, const std::vector<VxRunTimePoolInfo> *poolInfos);

    vx_status convertAllOperandsToRefences(const std::vector<VxRunTimePoolInfo>* poolInfos, vx_type_e target_format);

    vx_status getGraph(const std::vector<VxRunTimePoolInfo>* poolInfos);


    vx_status initVxRunTimePoolInfo(const std::vector<VxRunTimePoolInfo>* poolInfos);

    vx_tensor creatVirtualTensorByParam(vx_graph graph, vx_uint32 dimNum, vx_uint32 *dims, OperandType type, vx_float32 scale = 1.0f, int32_t zp = 0);
    vx_tensor creatVirtualTensorFromRTF(vx_graph graph, const VxRunTimeReferenceInfo &info);

    vx_context mContext = nullptr;

    vx_graph mGraph = nullptr;

    bool initializeRunTimeInfo(const std::vector<VxRunTimePoolInfo>& requestPoolInfos);

    const Model* mModel = nullptr;

    const Request* mRequest = nullptr;

    std::vector<VxOperationInfo> mOperationInfos;

};

namespace {

template <int N>
struct Dims {
    int sizes[N];
    int strides[N];
};


inline int vxcGetTypeSize(vx_enum format)
{
    switch(format)
    {
        case VX_TYPE_INT8:
        case VX_TYPE_UINT8:
            return 1;
        case VX_TYPE_INT16:
        case VX_TYPE_UINT16:
            return 2;
        case VX_TYPE_INT32:
        case VX_TYPE_UINT32:
            return 4;
        case VX_TYPE_INT64:
        case VX_TYPE_UINT64:
            return 8;
        case VX_TYPE_FLOAT32:
            return 4;
        case VX_TYPE_FLOAT64:
            return 8;
        case VX_TYPE_ENUM:
            return 4;
        case VX_TYPE_FLOAT16:
            return 2;
    }
    return 4;
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
inline void convertShapeToDims(const Shape& shape, std::vector<uint32_t>* _dims) {
    Dims<4> dims;
    for (int i = 0; i<4; i++) {
        dims.sizes[i] = 1;
    }

    if (shape.dimensions.size() == 1) {
        dims.sizes[0] = (int)getSizeOfDimension(shape, 0);
    }
    else {
        for (int i = 0; i<4; i++) {
            int src = (int)shape.dimensions.size() - i - 1;
            if (src >= 0) {
                dims.sizes[i] = (int)getSizeOfDimension(shape, src);
            }
        }
    }

    dims.strides[0] = 1;
    for (int i = 1; i<4; i++) {
        dims.strides[i] = dims.strides[i - 1] * dims.sizes[i - 1];
    }

    for (int i = 0; i<4; i++) {
        _dims->push_back(dims.sizes[i]);
    }

}

template <typename T>
T getScalarData(const VxRunTimeReferenceInfo& info) {
    if (info.buffer == nullptr)
        LOG(ERROR) << "getScalarData: buffer is null!";
    else
        LOG(ERROR) << "getScalarData: length = " << info.length << ", buffer = " << info.buffer;
    // TODO: Check buffer is at least as long as size of data.
    T* data = reinterpret_cast<T*>(info.buffer);
    return data[0];
}

#define F16_EXPONENT_BITS 0x1F
#define F16_EXPONENT_SHIFT 10
#define F16_EXPONENT_BIAS 15
#define F16_MANTISSA_BITS 0x3ff
#define F16_MANTISSA_SHIFT (23 - F16_EXPONENT_SHIFT)
#define F16_MAX_EXPONENT (F16_EXPONENT_BITS << F16_EXPONENT_SHIFT)

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

vx_enum getOvxActivationType(FusedActivationFunctionType type)
{
    vx_enum ovx_type[] = {
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_NONE,  /* kNone */
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU,  /* kRelu */
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU1, /* kRelu1 */
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU6, /* kRelu6 */
    };

    return ovx_type[(vx_uint32)type];
}

vx_bool isTensor(VxRunTimeReferenceInfo &info)
{
    if(info.type ==  OperandType::TENSOR_QUANT8_ASYMM || info.type ==  OperandType::TENSOR_FLOAT32 || info.type ==  OperandType::TENSOR_INT32)
        return vx_true_e;

    return vx_false_e;
}

vx_status convertScalar2Tensor(VxRunTimeReferenceInfo* info)
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

}  /* anonymous namespace */

} /* namespace nn */
} /* namespace android */

#endif /* ANDROID_ML_NN_COMMON_OVX_EXECUTOR_H */

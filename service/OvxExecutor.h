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

#include <pthread.h>

#include <VX/vx.h>
#include <VX/vx_api.h>
#include <VX/vx_khr_cnn.h>
#include <sys/time.h>
#ifdef VX_VERSION_1_2
#include <VX/viv_nn_compatibility.h>
#endif
#define VX_CHECK_ERROR(status) {\
    if(VX_SUCCESS != status){\
        LOG(ERROR)<<"status error"<< status<< " line:"<<__LINE__;\
        }\
}

#define HIGH_PRECISION_COMPUTE 1
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

    int initalize(vx_context* context, pthread_mutex_t* mutex, const Model* model, std::vector<VxRunTimePoolInfo>* poolInfos);

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

    vx_tensor creatTensorByParam(vx_context context, vx_uint32 dimNum, vx_uint32 *dims, OperandType type, vx_float32 scale, int32_t zp);

    void initalizeEnv();
    vx_status convertScalar2Tensor(VxRunTimeReferenceInfo* info);

    vx_context* mContext = nullptr;

    vx_context mPreContext = nullptr;

    vx_graph mGraph = nullptr;

    pthread_mutex_t* mMutex = nullptr;

    bool initializeRunTimeInfo(const std::vector<VxRunTimePoolInfo>& requestPoolInfos);

    const Model* mModel = nullptr;

    const Request* mRequest = nullptr;

    std::vector<VxOperationInfo> mOperationInfos;

    vx_bool preformaceDebug = vx_false_e;

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

inline double getTime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1.e-6;
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

inline vx_enum getOvxActivationType(FusedActivationFunctionType type)
{
    vx_enum ovx_type[] = {
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_NONE,  /* kNone */
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU,  /* kRelu */
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU1, /* kRelu1 */
        VX_CONVOLUTIONAL_NETWORK_ACTIVATION_RELU6, /* kRelu6 */
    };

    return ovx_type[(vx_uint32)type];
}

inline vx_bool isTensor(VxRunTimeReferenceInfo &info)
{
    if(info.type ==  OperandType::TENSOR_QUANT8_ASYMM || info.type ==  OperandType::TENSOR_FLOAT32 || info.type ==  OperandType::TENSOR_INT32)
        return vx_true_e;

    return vx_false_e;
}

}  /* anonymous namespace */

} /* namespace nn */
} /* namespace android */

#endif /* ANDROID_ML_NN_COMMON_OVX_EXECUTOR_H */

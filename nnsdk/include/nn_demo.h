/****************************************************************************
*
*    Copyright (c) 2019  by amlogic Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of amlogic Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of amlogic Corporation.
*
*****************************************************************************/

#ifndef _AMLNNDEMO_H
#define _AMLNNDEMO_H
#include "nn_sdk.h"
#include "nn_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=================================================
output of aml_module_t type:IMAGE_CLASSIFY
score: top 5 class confidence
top:   the class of top 5
=================================================*/
typedef struct __nn_image_classify
{
    float   score[5];
    unsigned int  topClass[5];
}img_classify_out_t;
/*=================================================
output of aml_module_t type:OBJECT_DETECT
detNum: the number of object detect
pBox:   the object info detect
=================================================*/
typedef struct __nn_obj_detect
{
    unsigned int  detNum;
    detBox *pBox;
}obj_detect_out_t;
/*=================================================
output of aml_module_t type:FACE_DETECTION
detNum: the number of face detect
pBox:   the face info detect
=================================================*/
typedef struct __nn_face_detect
{
    unsigned int  detNum;
    detBox pBox[MAX_DETECT_NUM];
}face_detect_out_t;
/*=================================================
output of aml_module_t type:FACE_LANDMARK_5
pos value:
0:left-eye,
1:right-eye,
2:nose,
3:left-mouth,
4:right-mouth
=================================================*/
typedef struct __nn_face_landmark_5
{
    unsigned int   detNum;
    detBox facebox[MAX_DETECT_NUM];
    point_t pos[MAX_DETECT_NUM][5];
}face_landmark5_out_t;

/*=================================================
output of aml_module_t type:FACE_LANDMARK_68
68 pos index:
0-16是下颌线，
17-21是右眼眉，
22-26是左眼眉，
27-35是鼻子，
36-41是右眼，
42-47是左眼，
48-60是嘴外轮廓，
61-67是嘴内轮廓。
=================================================*/
typedef struct __nn_face_landmark_68
{
    unsigned int detNum;
    detBox facebox[MAX_DETECT_NUM];
    point_t pos[MAX_DETECT_NUM][68];
}face_landmark68_out_t;
/*=================================================
output of aml_module_t type:FACE_RECOGNIZE
faceVector: the vector generate by input face,
can recognized by this vector
=================================================*/
typedef struct __nn_face_recognize
{
    float faceVector[512];
}face_recognize_out_t;
/*=================================================
output of aml_module_t type:FACE_COMPARISION
compare the input two image whether the same person or not:
compareScore: the score of compare two input face,
if compareScore >= 1.18, same person for input two face
if compareScore < 1.18,  not same person
=================================================*/
typedef struct __nn_face_compare
{
    float compareScore;
}face_compare_out_t;
/*=================================================
output of aml_module_t type:FACE_AGE
age value:
=================================================*/
typedef struct __nn_face_age
{
    int age;
}face_age_out_t;
/*=================================================
output of aml_module_t type:FACE_GENDER
female rate:
gender > 0.5: female
gender < 0.5: male
=================================================*/
typedef struct __nn_face_gender
{
    float gender;
}face_gender_out_t;
/*=================================================
output of aml_module_t type:FACE_EMOTION
0: 'angry',
1: 'disgust',
2: 'fear',
3: 'happy',
4: 'sad',
5: 'surprise',
6: 'neutral'
=================================================*/
typedef struct __nn_face_emotion
{
    int emotion;
    float prob;
}face_emotion_out_t;
/*=================================================
output of aml_module_t type:BODY_POSE
now just support single person pose
0:'Nose',
1:'Neck',
2:'R-Sho',
3:'R-Elb',
4:'R-Wr',
5:'L-Sho',
6:'L-Elb',
7:'L-Wr',
8:'R-Hip',
9:'R-Knee',
10:'R-Ank',
11:'L-Hip',
12:'L-Knee',
13:'L-Ank',
14:'R-Eye',
15:'L-Eye',
16:'R-Ear',
17:'L-Ear'
=================================================*/
typedef struct __nn_bodypos
{
    int valid;    //whether this point is valid
    point_t pos;
}bodypos_t;
typedef struct __nn_body_pose
{
    bodypos_t bpos[18];
}body_pose_out_t;


/*=================================================
output of aml_module_t type:FINGER_POSE
=================================================*/
typedef struct __nn_finger_pose
{
    image_out_t fingerOut;
}finger_pose_out_t;
/*=================================================
output of aml_module_t type:IMAGE_SR
=================================================*/
typedef struct __nn_sr_out
{
    image_out_t srOut;
}sr_out_t;
/*=================================================
output of aml_module_t type:IMAGE_SEGMENTATION
=================================================*/
typedef struct __nn_segment_out
{
    image_out_t segOut;
}segment_out_t;

/*=================================================
output of aml_module_t type:HEAD_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_head_detect
{
    obj_detect_out_t headOut;
}head_det_out_t;
/*=================================================
output of aml_module_t type:CARPLATE_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_car_detect
{
    obj_detect_out_t carOut;
}car_det_out_t;
/*=================================================
output of aml_module_t type:TEXT_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_text_detect
{
    obj_detect_out_t textOut;
}text_det_out_t;
/*=================================================
output of aml_module_t type:CARPLATE_RECOG
the output for car license confidence
confidence: the confidence of character for car license
val: the index number for license value;
10(0-9)+26(A-Z)+32(chinese character)
=================================================*/
typedef struct __nn_car_recognize
{
    float confidence;
    unsigned char val[32];
}car_license_out_t;
//////////////////////////////////////////////////////

typedef struct __nn_person_detect
{
    unsigned int  detNum;
    detBox pBox[MAX_DETECT_NUM];
}person_detect_out_t;

typedef struct __nn_yoloface_v2
{
    unsigned int  detNum;
    detBox *pBox;
}yoloface_v2_out_t;

typedef struct __nn_yolov2
{
    unsigned int  detNum;
    detBox *pBox;
}yolov2_out_t;

typedef struct __nn_yolov3
{
    unsigned int  detNum;
    detBox *pBox;
}yolov3_out_t;

typedef struct __nn_facenet
{
    unsigned char faceVector[128];
}facenet_out_t;

typedef struct __nn_face_recognize_uint
{
    unsigned char faceVector[512];
}face_recognize_uint_out_t;

typedef struct __nn_face_rfb_detect
{
    unsigned int   detNum;
    detBox facebox[MAX_DETECT_NUM];
    point_t pos[MAX_DETECT_NUM][5];
}face_rfb_detect_out_t;
/////////////////////common type//////////////////////

void softmax(float *input, int n, float temp, float *output);
void flatten(float *x, int size, int layers, int batch, int forward);
void do_nms_sort(box *boxes, float **probs, int total, int classes, float thresh);
int nms_comparator(const void *pa, const void *pb);
float box_iou(box a, box b);
float box_union(box a, box b);
float box_intersection(box a, box b);
float overlap(float x1, float w1, float x2, float w2);
float logistic_activate(float x);
float sigmod(float x);
unsigned char *transpose(const unsigned char * src,int width,int height);
void process_top5(float *buf,unsigned int num,img_classify_out_t* clsout);
void *post_process_all_module(aml_module_t type,nn_output *pOut);
int max_index(float *a, int n);
box get_region_box(float *x, float *biases, int n, int index, int i, int j, int w, int h);

#ifdef __cplusplus
} //extern "C"
#endif
#endif // _AMLNNSDK_H

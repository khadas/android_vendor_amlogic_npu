/****************************************************************************
*   amlogic nn api util header file
*
*   Neural Network appliction network definition some util header file
*
*   Date: 2019.8
****************************************************************************/
#ifndef _AMLOGIC_NN_UTIL_H
#define _AMLOGIC_NN_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_IDLE          0
#define POWER_ON            1
#define POWER_SUSPEND       2
#define POWER_OFF           3
#define POWER_RESET         4

typedef struct{
    float x, y;
} landmark;
/*
typedef struct{
    float x, y, w, h;
} box;
*/
typedef struct{
    float x, y, w, h, prob_obj;
} box;
typedef struct{
    int index;
    int classId;
    float **probs;
} sortable_bbox;

///////////////////////////////////////some util api///////////////////////////////////////////////
unsigned char *get_jpeg_rawData(const char *name,unsigned int width,unsigned int height);
int init_fb(void);
void *camera_thread_func(void *arg);
int sysfs_control_read(const char* name,char *out);
int sysfs_control_write(const char* pname,char *value);
int findtok(const char *buff,const char token,int lenth);
#ifdef __cplusplus
} //extern "C"
#endif
#endif
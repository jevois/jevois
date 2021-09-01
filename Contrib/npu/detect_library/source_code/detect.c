#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h> 

#include "nn_detect.h"
#include "nn_detect_utils.h"
#include "detect_log.h"


typedef unsigned char   uint8_t;

#define _SET_STATUS_(status, stat, lbl) do {\
	status = stat; \
	goto lbl; \
}while(0)

#define _CHECK_STATUS_(status, lbl) do {\
	if (NULL == status) {\
		goto lbl; \
	} \
}while(0)

typedef enum {
	NETWORK_UNINIT,
	NETWORK_INIT,
	NETWORK_PREPARING,
	NETWORK_PROCESSING,
} network_status;

typedef det_status_t (*network_create)(const char * data_file_path, dev_type type);
typedef void (*network_getsize)(int *width, int *height, int *channel);
typedef void (*network_setinput)(input_image_t imageData, uint8_t* data);
typedef det_status_t (*network_getresult)(pDetResult resultData, uint8_t* data);
typedef void (*network_release)(dev_type type);

typedef struct function_process {
	network_create		model_create;
	network_getsize		model_getsize;
	network_setinput	model_setinput;
	network_getresult	model_getresult;
	network_release		model_release;
}network_process;

typedef struct detect_network {
	uint8_t				*input_ptr;
	network_status		status;
	network_process 	process;
	void *				handle_id;
}det_network_t, *p_det_network_t;

dev_type g_dev_type = DEV_REVA;
det_network_t network[DET_BUTT]={0};

const char* data_file_path = "/jevoispro/share/npu/detection";
const char * so_file_name[DET_BUTT]= {
	"libnn_yoloface.so",
	"libnn_yolo_v2.so",
	"libnn_yolo_v3.so",
	"libnn_yolo_tiny.so",
	"libnn_yolo_v4.so",
	"libnn_ssd.so",
	"libnn_mtcnn_v1.so",
	"libnn_mtcnn_v2.so",
	"libnn_faster_rcnn.so",
	"libnn_deeplab_v1.so",
	"libnn_deeplab_v2.so",
	"libnn_deeplab_v3.so",
	"libnn_facenet.so",
};

static det_status_t check_input_param(input_image_t imageData, det_model_type modelType)
{
	int ret = DET_STATUS_OK;
	if (NULL == imageData.data) {
		LOGE("Data buffer is NULL");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	if (imageData.pixel_format != PIX_FMT_NV21 && imageData.pixel_format != PIX_FMT_RGB888) {
		LOGE("Current only support RGB888 and NV21");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	int width,height,channel;
	ret = det_get_model_size(modelType, &width, &height, &channel);
	if (ret) {
		LOGE("Get_model_size fail!");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	if (width != imageData.width) {
		LOGE("Inputsize not match! net VS img is width:%dvs%d \n",
				width, imageData.width);
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	if (imageData.pixel_format == PIX_FMT_RGB888) {
		if (channel != imageData.channel || height != imageData.height) {
			LOGE("Inputsize not match! net VS img is height:%dvs%d, width:channel:%dvs%d \n", height, imageData.height, channel, imageData.channel);
			_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
		}
	}

exit:
	return ret;
}

static int find_string_index(char* buffer, char* str, int lenght)
{
	int index = 0;
	int n = strlen(str);
	int i, j;

	for (i = lenght -n -1; i >0 ; i--) {
		for (j=0; j<n; j++) {
			if (buffer[i+j] != str[j]) {
				break;
			}
		}
		if (j == n) {
			index = i;
			break;
		}
	}

	return index;
}

static void check_and_set_dev_type()
{
	#define len 1280
	int index = 0;
	char buffer[len] = {0};
	memset(buffer, 0, sizeof(buffer));

	FILE* fp = popen("cat /proc/cpuinfo", "r");
	if (!fp) {
		LOGW("Popen /proc/cupinfo fail,set default RevA");
		g_dev_type = DEV_REVA;
		return ;
	}
	int length = fread(buffer,1,len,fp);
	index = find_string_index(buffer, "290", length);

	LOGD("Read Cpuinfo:\n%s\n",buffer);
	LOGD("290 index=%d",index);
	switch (buffer[index+3]) {
		case 'a':
			LOGI("set_dev_type REVA and setenv 0");
			g_dev_type = DEV_REVA;
			setenv("DEV_TYPE", "0", 0);
			break;
		case 'b':
			LOGI("set_dev_type REVB and setenv 1");
			g_dev_type = DEV_REVB;
			setenv("DEV_TYPE", "1", 0);
			break;
		default:
			LOGI("set_dev_type DEV_MS1 and setenv 2 index=%d char:%c", index, buffer[index+3]);
			g_dev_type = DEV_MS1;
			setenv("DEV_TYPE", "2", 0);
			break;
	}
	pclose(fp);
}

det_status_t check_and_set_function(det_model_type modelType)
{
	LOGP("Enter, dlopen so:%s", so_file_name[modelType]);

	int ret = DET_STATUS_ERROR;
	p_det_network_t net = &network[modelType];

	net->handle_id =  dlopen(so_file_name[modelType], RTLD_NOW);
	if (NULL == net->handle_id) {
		LOGE("dlopen %s failed!", so_file_name[modelType]);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	net->process.model_create = (network_create)dlsym(net->handle_id, "model_create");
	_CHECK_STATUS_(net->process.model_create, exit);

	net->process.model_getsize = (network_getsize)dlsym(net->handle_id, "model_getsize");
	_CHECK_STATUS_(net->process.model_getsize, exit);

	net->process.model_setinput = (network_setinput)dlsym(net->handle_id, "model_setinput");
	_CHECK_STATUS_(net->process.model_setinput, exit);

	net->process.model_getresult = (network_getresult)dlsym(net->handle_id, "model_getresult");
	_CHECK_STATUS_(net->process.model_getresult, exit);

	net->process.model_release = (network_release)dlsym(net->handle_id, "model_release");
	_CHECK_STATUS_(net->process.model_release, exit);

	ret = DET_STATUS_OK;
exit:
	LOGP("Leave, dlopen so:%s, ret=%d", so_file_name[modelType], ret);
	return ret;
}

det_status_t det_set_model(det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);

	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];

	if (modelType >= DET_BUTT) {
		LOGE("Det_set_model fail, modelType >= BUTT");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	check_and_set_dev_type();
	/*dlopen so ,and found process founction pointer by symbol */
	ret = check_and_set_function(modelType);
	if (ret) {
		LOGE("ModelType so open failed or Not support now!!");
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	LOGI("Start create Model, data_file_path=%s",data_file_path);
	ret = net->process.model_create(data_file_path, g_dev_type);
	if (ret) {
		LOGE("Model_create fail, file_path=%s, dev_type=%d", data_file_path, g_dev_type);
		_SET_STATUS_(ret, DET_STATUS_CREATE_NETWORK_FAIL, exit);
	}
	net->status = NETWORK_INIT;

	int width,height,channel;
	ret = det_get_model_size(modelType, &width, &height, &channel);
	if (ret) {
		LOGE("Get_model_size fail!");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	net->input_ptr = (uint8_t*) malloc(width * height * channel * sizeof(uint8_t));
	LOGI("input_ptr size=%d, addr=%x", width * height * channel * sizeof(uint8_t), net->input_ptr);
exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_get_model_size(det_model_type modelType, int *width, int *height, int *channel)
{
	LOGP("Enter, modeltype:%d", modelType);

	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (!net->status) {
		LOGE("Model has not created! modeltype:%d", modelType);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	net->process.model_getsize(width, height, channel);
exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_set_input(input_image_t imageData, det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);
	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (!net->status) {
		LOGE("Model has not created! modeltype:%d", modelType);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	ret = check_input_param(imageData, modelType);
	if (ret) {
		LOGE("Check_input_param fail.");
		_SET_STATUS_(ret, DET_STATUS_PARAM_ERROR, exit);
	}

	net->process.model_setinput(imageData, net->input_ptr);
	net->status = NETWORK_PREPARING;

exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_get_result(pDetResult resultData, det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);

	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (NETWORK_PREPARING != net->status) {
		LOGE("Model not create or not prepared! status=%d", net->status);
		_SET_STATUS_(ret, DET_STATUS_ERROR, exit);
	}

	ret = net->process.model_getresult(resultData, net->input_ptr);
	if (ret) {
		LOGE("Process Net work fail");
		_SET_STATUS_(ret, DET_STATUS_PROCESS_NETWORK_FAIL, exit);
	}
	net->status = NETWORK_INIT;

exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_release_model(det_model_type modelType)
{
	LOGP("Enter, modeltype:%d", modelType);
	int ret = DET_STATUS_OK;
	p_det_network_t net = &network[modelType];
	if (!net->status) {
		LOGW("Model has benn released!");
		_SET_STATUS_(ret, DET_STATUS_OK, exit);
	}

	net->process.model_release(g_dev_type);
	if (net->input_ptr) {
		free(net->input_ptr);
		net->input_ptr = NULL;
	}

	dlclose(net->handle_id);
	net->handle_id = NULL;
	net->status = NETWORK_UNINIT;
exit:
	LOGP("Leave, modeltype:%d", modelType);
	return ret;
}

det_status_t det_set_log_config(det_debug_level_t level,det_log_format_t output_format)
{
	LOGP("Enter, level:%d", level);
	det_set_log_level(level, output_format);
	LOGP("Leave, level:%d", level);
	return 0;
}

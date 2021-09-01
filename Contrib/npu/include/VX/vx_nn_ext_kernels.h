/*
 * vx_nn_ext_kernels.h
 *
 *  Created on: 2019-10-28
 *      Author: vsadmin
 */

#ifndef VX_NN_EXT_KERNELS_H_
#define VX_NN_EXT_KERNELS_H_

#include <stdint.h>
#include <VX/vx.h>
#include <VX/vx_helper.h>
#include "VX/viv_nn_compatibility.h"
#include "VX/vx_ext.h"

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define NN_DIV_DS_SIZE_ROUNDING(a, b, p) \
    (((p) == VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_CEILING) ? (((a) + (b) - 1) / (b)) : ((a) / (b)))

//#define LOG_ERROR(status, format, ...)
//#define LOG_WARN(status, format, ...)
//#define LOG_INFO(status, format, ...)
//#define LOG_DEBUG(status, format, ...)

#ifndef LOG_ERROR
#define LOG_ERROR(format, ...) \
	do{ \
		printf( "[ERROR] " format "%s:%d", ##__VA_ARGS__, __FILE__, __LINE__ ); \
		fflush(stdout); \
	}while(0)
#endif
#ifndef LOG_WARN
#define LOG_WARN(format, ...)  do{printf("[WARN ] " format, __VA_ARGS__); fflush(stdout);}while(0)
#endif
#ifndef LOG_INFO
#define LOG_INFO(format, ...)  do{printf("[INFO ] " format, __VA_ARGS__); fflush(stdout);}while(0)
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG(format, ...) do{printf("[DEBUG] " format, __VA_ARGS__); fflush(stdout);}while(0)
#endif

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT	
#endif

#define vxcMIN(x, y)            (((x) <= (y)) ?  (x) :  (y))
#define vxcMAX(x, y)            (((x) >= (y)) ?  (x) :  (y))

#ifndef VXC_VERIFY_EXPR
#define VXC_VERIFY_EXPR(a, label) \
    do {\
        int r = (int)(a);\
        if (!r) {\
        	LOG_DEBUG("%s %d: false expression (%s)\n", __FUNCTION__, __LINE__, #a);\
            goto label;\
        }\
    } while (0)
#endif

#ifndef VXC_VERIFY_STATUS
#define VXC_VERIFY_STATUS(a, label) \
    do {\
        int r = (a);\
        if (r != VX_SUCCESS) {\
        	LOG_DEBUG("%s %d: error status (%d)\n", __FUNCTION__, __LINE__, r);\
            goto label;\
        }\
    } while (0)
#endif

#ifndef VXC_VERIFY_PTR
#define VXC_VERIFY_PTR(a, label) \
    do {\
        if ((a) == NULL) {\
        	LOG_DEBUG("%s %d: NULL pointer\n", __FUNCTION__, __LINE__);\
            goto label;\
        }\
    } while (0)
#endif

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#define MAX_TENSOR_DIMENSION 6

#define VX_KERNEL_NAME_ACTIVATION 						"com.vivantecorp.nn.extension.activation"
#define VX_KERNEL_NAME_POOLING 							"com.vivantecorp.nn.extension.pooling"
#define VX_KERNEL_NAME_CONVOLUTION 						"com.vivantecorp.nn.extension.convolution"
#define VX_KERNEL_NAME_FULLYCONNECTED 					"com.vivantecorp.nn.extension.fullyConnected"
#define VX_KERNEL_NAME_SOFTMAX						 	"com.vivantecorp.nn.extension.softmax"
#define VX_KERNEL_NAME_NORMALIZATION 					"com.vivantecorp.nn.extension.normalization"
#define VX_KERNEL_NAME_NORMALIZATION2 					"com.vivantecorp.nn.extension.normalization2"
#define VX_KERNEL_NAME_CONVOLUTION_RELU_POOLING 		"com.vivantecorp.nn.extension.convolutionReluPooling"
#define VX_KERNEL_NAME_FULLYCONNECTED_RELU 				"com.vivantecorp.nn.extension.fullyConnectedRelu"
#define VX_KERNEL_NAME_DEPTHWISECONVOLUTION 			"com.vivantecorp.nn.extension.depthwiseConvolution"
#define VX_KERNEL_NAME_BATCHNORMALIZATION 				"com.vivantecorp.nn.extension.batchnormalization"
#define VX_KERNEL_NAME_TENSORADD 						"com.vivantecorp.nn.extension.tensorAdd"
#define VX_KERNEL_NAME_TENSORSUBTRACT 					"com.vivantecorp.nn.extension.tensorSubtract"
#define VX_KERNEL_NAME_TENSORMULTIPLY 					"com.vivantecorp.nn.extension.tensorMultiply"
#define VX_KERNEL_NAME_TENSORDIVIDE 					"com.vivantecorp.nn.extension.tensorDivide"
#define VX_KERNEL_NAME_PERMUTE 							"com.vivantecorp.nn.extension.permute"
#define VX_KERNEL_NAME_TENSORREVERSE 					"com.vivantecorp.nn.extension.tensorReverse"
#define VX_KERNEL_NAME_TENSORREDUCESUM 					"com.vivantecorp.nn.extension.tensorReduceSum"
#define VX_KERNEL_NAME_SLICE 							"com.vivantecorp.nn.extension.slice"
#define VX_KERNEL_NAME_SLICE2 							"com.vivantecorp.nn.extension.slice2"
#define VX_KERNEL_NAME_SPLIT 							"com.vivantecorp.nn.extension.split"
#define VX_KERNEL_NAME_UNPACK 							"com.vivantecorp.nn.extension.unpack"
#define VX_KERNEL_NAME_CONCAT 							"com.vivantecorp.nn.extension.concat"
#define VX_KERNEL_NAME_DECONVOLUTION 					"com.vivantecorp.nn.extension.deconvolution"
#define VX_KERNEL_NAME_ROIPOOLING 						"com.vivantecorp.nn.extension.ROIPooling"
#define VX_KERNEL_NAME_REORG 							"com.vivantecorp.nn.extension.reorg"
#define VX_KERNEL_NAME_PROPOSAL 						"com.vivantecorp.nn.extension.proposal"

enum user_library_e
{
	VX_LIBRARY_LIBVXNNEXT        = 1,
};
enum user_kernel_e
{
	VX_KERNEL_ENUM_ACTIVATION      				= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x001,
	VX_KERNEL_ENUM_POOLING     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x002,
	VX_KERNEL_ENUM_CONVOLUTION     				= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x003,
	VX_KERNEL_ENUM_FULLYCONNECTED     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x004,
	VX_KERNEL_ENUM_SOFTMAX     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x005,
	VX_KERNEL_ENUM_NORMALIZATION     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x006,
	VX_KERNEL_ENUM_NORMALIZATION2     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x007,
	VX_KERNEL_ENUM_CONVOLUTION_RELU_POOLING     = VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x008,
	VX_KERNEL_ENUM_FULLYCONNECTEDR_RELU     	= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x009,
	VX_KERNEL_ENUM_DEPTHWISECONVOLUTION     	= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x010,
	VX_KERNEL_ENUM_BATCHNORMALIZATION     		= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x011,
	VX_KERNEL_ENUM_TENSORADD     				= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x012,
	VX_KERNEL_ENUM_TENSORSUBTRACT     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x013,
	VX_KERNEL_ENUM_TENSORMULTIPLY     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x014,
	VX_KERNEL_ENUM_TENSORDIVIDE    				= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x015,
	VX_KERNEL_ENUM_PERMUTE     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x016,
	VX_KERNEL_ENUM_TENSORREVERSE     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x017,
	VX_KERNEL_ENUM_TENSORREDUCESUM     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x018,
	VX_KERNEL_ENUM_SLICE     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x019,
	VX_KERNEL_ENUM_SLICE2     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x020,
	VX_KERNEL_ENUM_SPLIT     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x021,
	VX_KERNEL_ENUM_UNPACK     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x022,
	VX_KERNEL_ENUM_CONCAT     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x023,
	VX_KERNEL_ENUM_DECONVOLUTION     			= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x024,
	VX_KERNEL_ENUM_ROIPOOLING     				= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x0025,
	VX_KERNEL_ENUM_REORG     					= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x0026,
	VX_KERNEL_ENUM_PROPOSAL     				= VX_KERNEL_BASE( VX_ID_DEFAULT, VX_LIBRARY_LIBVXNNEXT ) + 0x0027,
};

vx_status VX_CALLBACK vxDefaultInitializer(vx_node node, const vx_reference *parameters, vx_uint32 num);

vx_status VX_CALLBACK vxDefaultDeinitializer(vx_node node, const vx_reference *parameters, vx_uint32 num);

vx_status VX_CALLBACK vxDefaultValidator( vx_node node,
                                             const vx_reference parameters[], vx_uint32 num,
                                             vx_meta_format metas[] );

DLLEXPORT vx_status setNodeId(int id);
int getNodeId();

DLLEXPORT vx_status initNNExtensions( vx_context context, const char* qfile );

vx_status registerPoolingKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtPoolingLayer( vx_graph graph, vx_tensor input,
	    const vx_nn_pooling_params_t * pooling_params, vx_size size_of_pooling_params, vx_tensor outputs);

vx_status registerActivationKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtActivationLayer(vx_graph graph, vx_tensor inputs,
		vx_enum func, vx_float32 a, vx_float32 b, vx_tensor outputs);

vx_status registerSoftmaxKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtSoftmaxLayer(vx_graph graph, vx_tensor inputs, vx_tensor outputs);

vx_status registerDepthwiseConvolutionKernel( vx_context context );
vx_status registerConvolutionKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtConvolutionLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights, vx_tensor biases,
		const vx_nn_convolution_params_t *conv_basic_params, vx_size size_of_convolution_params, vx_tensor outputs);

vx_status registerFullyConnectedKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtFullyConnectedLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights,
	    vx_tensor biases, vx_uint32 pad, vx_uint8 accumulator_bits, vx_enum overflow_policy,
	    vx_enum rounding_policy, vx_enum down_scale_size_rounding, vx_tensor outputs);

vx_status registerNormalizationKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtNormalizationLayer(vx_graph graph, vx_tensor inputs, vx_enum type,
        vx_size norm_size, vx_float32 alpha, vx_float32 beta, vx_tensor outputs);

vx_status registerNormalization2Kernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtNormalizationLayer2(vx_graph graph, vx_tensor inputs,
		const vx_nn_normalization_params_t *normalization_params,
	    vx_size size_of_normalization_param, vx_tensor outputs);

vx_status registerConvolutionReluPoolingKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtConvolutionReluPoolingLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights, vx_tensor biases,
	    const vx_nn_convolution_relu_pooling_params_t * convolution_relu_pooling_basic_params,
	    vx_size size_of_convolution_relu_pooling_params, vx_tensor outputs);

vx_status registerFullyConnectedReluKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtFullyConnectedReluLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights,
	    vx_tensor biases, vx_uint32 pad, vx_uint8 accumulator_bits, vx_enum overflow_policy,
	    vx_enum rounding_policy, vx_enum down_scale_size_rounding, vx_bool enable_relu, vx_tensor outputs);

vx_status registerBatchNormalizationKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtBatchNormalizationLayer(vx_graph graph, vx_float32 eps,
	    vx_tensor mean, vx_tensor variance, vx_tensor gamma, vx_tensor beta,
	    vx_tensor input, vx_tensor output);

vx_status registerTensorAddKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorAddLayer(vx_graph graph, vx_tensor input1, vx_tensor input2,
		vx_enum policy, vx_tensor output);

vx_status registerTensorSubtractKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorSubtractLayer(vx_graph graph, vx_tensor input1, vx_tensor input2,
		vx_enum policy, vx_tensor output);

vx_status registerTensorMultiplyKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorMultiplyLayer(vx_graph graph, vx_tensor input1, vx_tensor input2,
		vx_scalar scale, vx_enum overflow_policy, vx_enum rounding_policy, vx_tensor output);

vx_status registerTensorDivideKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorDivideLayer(vx_graph graph, vx_tensor input1, vx_tensor input2,
		vx_scalar scale, vx_enum overflow_policy, vx_enum rounding_policy, vx_tensor output);

vx_status registerTensorPermuteKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorPermuteLayer(vx_graph graph, vx_tensor inputs,
		vx_tensor outputs, vx_uint32* perm, vx_uint32 sizes_of_perm);

vx_status registerTensorReverseKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorReverseLayer(vx_graph graph, vx_tensor inputs,
		const vx_nn_tensor_reverse_params_t * tensor_reverse_params,
		vx_size size_of_tensor_reverse_params, vx_tensor outputs);

vx_status registerTensorReduceSumKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtTensorReduceSumLayer(vx_graph graph, vx_tensor in, vx_tensor out,
		vx_uint32* reduce_dim, vx_int32 dim_size, vx_bool keep_dim);

vx_status registerSliceKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtSliceLayer(vx_graph graph, vx_tensor inputs, vx_int32 axis, vx_int32 number,
		vx_int32 *slice, vx_object_array outputs);

vx_status registerSlice2Kernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtSlice2Layer(vx_graph graph, vx_tensor inputs,
	    const vx_nn_slice2_params_t *slice2_params, vx_size size_of_slice2_params, vx_tensor outputs);

vx_status registerSplitKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtSplitLayer(vx_graph graph, vx_tensor inputs, vx_int32 axis,
		vx_int32 number, vx_object_array outputs);

vx_status registerUnpackKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtUnpackLayer(vx_graph graph, vx_tensor inputs, vx_int32 axis,
		vx_int32 number, vx_object_array outputs);

vx_status registerConcatKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtConcatIndefiniteLayer(vx_graph graph, vx_object_array in,
    const vx_nn_concat_params_t* concat_params, vx_size size_of_concat_params, vx_tensor out);

vx_status registerDeconvolutionKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtDeconvolutionLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights, vx_tensor  biases,
	const vx_nn_deconvolution_params_t *deconvolution_basic_params, vx_size size_of_deconv_params, vx_tensor outputs);

vx_status registerROIPoolingKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtROIPoolingLayer(vx_graph graph, vx_tensor input_data, vx_tensor input_rois,
	const vx_nn_roi_pool_params_t *roi_pool_basic_params, vx_size size_of_roi_params, vx_tensor output_arr);

vx_status registerReorgKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtReorgLayer(vx_graph graph, vx_tensor inputs, vx_uint32 stride, vx_tensor outputs);

vx_status registerProposalKernel( vx_context context );
VX_API_ENTRY vx_node VX_API_CALL vxExtRPNLayer(vx_graph graph, vx_tensor score, vx_tensor bbox, vx_tensor anchors, vx_tensor img_info,
    const vx_nn_rpn_params_t *  rpn_params, vx_size size_of_rpn_params, vx_tensor roi_output, vx_tensor score_output);

#ifdef __cplusplus
}
#endif

#endif /* VX_NN_EXT_KERNELS_H_ */

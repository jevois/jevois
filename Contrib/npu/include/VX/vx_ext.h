/*******************************************************************************
 * Copyright (c) 2016, 2026 VeriSilicon Holdings Co., Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the VeriSilicon Product License v1.0
 * which accompanies this distribution, and is available at
 * http://www.verisilicon.com/legal/vpl-v10.html
 *
 * Contributors:
 *     Vic Zhang (VeriSilicon) - initial API and implementation
 *******************************************************************************/
#ifndef _VX_VSI_NN_EXT_H_
#define _VX_VSI_NN_EXT_H_

/*!
 * \file
 * \brief The Khronos Extension for Deep Convolutional Networks Functions.
 *
 * \defgroup group_cnn Extension: Deep Convolutional Networks API
 * \brief Convolutional Network Nodes.
 */

#define OPENVX_VSI_NNe   "vx_vsi_nne"

#include <VX/vx.h>
#include <VX/vx_ext_program.h>
#include <VX/vx_helper.h>
#include <VX/vxu.h>


#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _vx_nn_argmax_params_t
{
    vx_int32 axis;
} vx_nn_argmax_params_t;

typedef struct _vx_nn_imageresize_params_t
{
    vx_enum type;
    vx_float32 factor;
    vx_int32 width;
    vx_int32 height;
} vx_nn_imageresize_params_t;

typedef struct _vx_nn_instancenormalization_params_t
{
    vx_float32 eps;
} vx_nn_instancenormalization_params_t;

typedef struct _vx_nn_l2normalization_scale_params_t
{
    vx_int32 axis;
} vx_nn_l2normalization_scale_params_t;

typedef struct _vx_nn_pool_argmax_params_t
{
    vx_int32 ksize_x;
    vx_int32 ksize_y;
    vx_int32 stride_x;
    vx_int32 stride_y;
    vx_int32 pad_x;
    vx_int32 pad_x_right;
    vx_int32 pad_y;
    vx_int32 pad_y_bottom;
    vx_enum pool_type;
    vx_enum rounding_policy;
} vx_nn_pool_argmax_params_t;

typedef struct _vx_nn_slice2_params_t
{
    vx_int32 *begin;
    vx_int32 *length;
} vx_nn_slice2_params_t;

typedef struct _vx_nn_upsample_params_t
{
    vx_int32 width;
    vx_int32 height;
} vx_nn_upsample_params_t;

VX_API_ENTRY vx_status VX_API_CALL vxNNExtInit(
        vx_context context
        );

VX_API_ENTRY vx_node VX_API_CALL vxArgMaxLayer(
    vx_graph                    graph,
    vx_tensor                   inputs,
    const vx_nn_argmax_params_t *argmax_params,
    vx_size                     size_of_argmax_params,
    vx_tensor                   outputs
    );

VX_API_ENTRY vx_node VX_API_CALL vxInstanceNormalizationLayer(
    vx_graph                    graph,
    vx_tensor                   inputs,
    vx_tensor                   scales,
    vx_tensor                   biases,
    vx_tensor                   axises,
    const vx_nn_instancenormalization_params_t *instancenormalization_params,
    vx_size                     size_of_instancenormalization_params,
    vx_tensor                   outputs
    );

VX_API_ENTRY vx_node VX_API_CALL vxL2normalizationScaleLayer(
    vx_graph                    graph,
    vx_tensor                   inputs,
    vx_tensor                   scales,
    const vx_nn_l2normalization_scale_params_t *l2normalization_scale_params,
    vx_size                     size_of_l2normalization_scale_params,
    vx_tensor                   outputs
    );

VX_API_ENTRY vx_node VX_API_CALL vxPoolArgmaxLayer(
    vx_graph                    graph,
    vx_tensor                   inputs,
    const vx_nn_pool_argmax_params_t *pool_argmax_params,
    vx_size                     size_of_pool_argmax_params,
    vx_tensor                   outputs,
    vx_tensor                   indexes
    );

VX_API_ENTRY vx_node VX_API_CALL vxSlice2Layer(
    vx_graph                    graph,
    vx_tensor                   inputs,
    const vx_nn_slice2_params_t *slice2_params,
    vx_size                     size_of_slice2_params,
    vx_tensor                   outputs
    );

VX_API_ENTRY vx_node VX_API_CALL vxUpsampleLayer(
    vx_graph                    graph,
    vx_tensor                   inputs,
    vx_tensor                   indexes,
    const vx_nn_upsample_params_t *upsample_params,
    vx_size                     size_of_upsample_params,
    vx_tensor                   outputs
    );

#ifdef  __cplusplus
}
#endif

#endif

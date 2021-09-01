/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#ifndef _VSI_NN_OP_GRUCELL_OVXLIB_H
#define _VSI_NN_OP_GRUCELL_OVXLIB_H

#include "vsi_nn_types.h"

#define GRUCELL_RZ_GATE_COUNT 2

/* enum for inputs/outputs */
enum
{
    GRUCELL_INPUT_INPUT        = 0,
    GRUCELL_INPUT_H_STATE      = 1,

    GRUCELL_INPUT_WEIGHT_I2R   = 2,
    GRUCELL_INPUT_WEIGHT_I2Z   = 3,

    GRUCELL_INPUT_WEIGHT_H2R   = 4,
    GRUCELL_INPUT_WEIGHT_H2Z   = 5,

    GRUCELL_INPUT_BIAS_R       = 6,
    GRUCELL_INPUT_BIAS_Z       = 7,

    GRUCELL_INPUT_WEIGHT_I2C   = 8,
    GRUCELL_INPUT_WEIGHT_R2C   = 9,
    GRUCELL_INPUT_BIAS_C       = 10,

    GRUCELL_INPUT_CNT,

    GRUCELL_OUTPUT_OUTPUT      = 0,
    GRUCELL_OUTPUT_H_STATE     = 1,

    GRUCELL_OUTPUT_CNT
};

enum
{
    GRUCELL_QUANTIZE_PARAM_I2R,
    GRUCELL_QUANTIZE_PARAM_I2Z,
    GRUCELL_QUANTIZE_PARAM_H2R,
    GRUCELL_QUANTIZE_PARAM_H2Z,
    GRUCELL_QUANTIZE_PARAM_I2C,
    GRUCELL_QUANTIZE_PARAM_R2C,

    GRUCELL_QUANTIZE_PARAM_COUNT
};

enum
{
    GRUCELL_GATE_R = 0,
    GRUCELL_GATE_Z = 1,

    GRUCELL_GATE_COUNT
};

typedef struct _vsi_nn_grucell_ovxlib_lcl_data_t
{
    vsi_nn_activation_e gate_activation;
    vsi_nn_activation_e candidate_activation;
    vsi_bool multi_batch;
} vsi_nn_grucell_ovxlib_lcl_data_t;

typedef struct _vsi_nn_grucell_ovxlib_param
{
    vsi_nn_grucell_ovxlib_lcl_data_t local;

    vsi_nn_activation_e activation;
    vsi_nn_dtype_t internal_dtype[GRUCELL_QUANTIZE_PARAM_COUNT];
} vsi_nn_grucell_ovxlib_param;

#endif

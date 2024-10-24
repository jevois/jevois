/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "common/hailo_objects.hpp"
#include "common/hailo_common.hpp"

#include <xtensor/xview.hpp>
#include <xtensor/xsort.hpp>

struct KeyPt {
    float xs;
    float ys;
    float joints_scores;
};

struct PairPairs {
    std::pair<float, float> pt1;
    std::pair<float, float> pt2;
    float s1;
    float s2;
};

struct Triple {
    std::vector<HailoTensorPtr> boxes;
    xt::xarray<float> scores;
    std::vector<HailoTensorPtr> keypoints;
};

struct Decodings {
    HailoDetection detection_box;
    std::pair<xt::xarray<float>, xt::xarray<float>> keypoints;
    std::vector<PairPairs> joint_pairs;
};


__BEGIN_DECLS
//not used by jevois
//std::pair<std::vector<KeyPt>, std::vector<PairPairs>> filter(HailoROIPtr roi);

// JEVOIS: filter() has hardwired constants. We use the lower-level functions instead:
std::vector<Decodings> yolov8pose_postprocess(std::vector<HailoTensorPtr> &tensors,
                                              std::vector<int> network_dims,
                                              std::vector<int> strides,
                                              int regression_length,
                                              int num_classes, float score_threshold, float iou_threshold);
std::pair<std::vector<KeyPt>, std::vector<PairPairs>> filter_keypoints(std::vector<Decodings> filtered_decodings,
                                                                       std::vector<int> network_dims,
                                                                       float joint_threshold=0.5);


__END_DECLS

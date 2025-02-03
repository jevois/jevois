// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2024 by Laurent Itti, the University of Southern
// California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
//
// This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
// redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
// Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.  You should have received a copy of the GNU General Public License along with this program;
// if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
// Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! \file */

#ifdef JEVOIS_PRO

#include <jevois/DNN/CLIP.H>
#include <jevois/Debug/Log.H>
#include <clip.cpp/clip.h>

#define CLIP_THREADS 4

// ####################################################################################################
jevois::dnn::CLIP::~CLIP()
{
  if (itsCtx) clip_free(itsCtx);
}

// ####################################################################################################
jevois::dnn::CLIP::CLIP(std::string const & fname)
{
  if (itsCtx) clip_free(itsCtx);

  LINFO("Loading CLIP model " << fname << " ...");

  itsCtx = clip_model_load(fname.c_str(), 0 /* verbosity */);
  if (itsCtx == nullptr) LFATAL("Failed to load model from " << fname);

  LINFO("CLIP model ready.");
}

// ####################################################################################################
cv::Mat jevois::dnn::CLIP::textEmbedding(std::string const & txt)
{
  // First get tokens from text:
  clip_tokens tokens;
  if (!clip_tokenize(itsCtx, txt.c_str(), &tokens)) LFATAL("Failed to tokenize [" << txt << ']');

  // Then get embedding from the tokens:
  int const vec_dim = clip_get_text_hparams(itsCtx)->projection_dim;
  cv::Mat ret(1, vec_dim, CV_32F); float * vec = (float *)ret.data;

  if (!clip_text_encode(itsCtx, CLIP_THREADS, &tokens, vec, false)) LFATAL("Failed to encode text [" << txt << ']');

  // Standardize to unit norm, as expected by YOLO-JeVois:
  float const norm = cv::norm(ret);
  ret /= norm;
  
  return ret;
}

// ####################################################################################################
int jevois::dnn::CLIP::textEmbeddingSize() const
{
  if (itsCtx == nullptr) LFATAL("No CLIP model loaded");
  if (clip_model_has_text_encoder(itsCtx) == false) return 0;
  return clip_get_text_hparams(itsCtx)->projection_dim;
}

// ####################################################################################################
cv::Mat jevois::dnn::CLIP::imageEmbedding(cv::Mat const & img)
{
  if (itsCtx == nullptr) LFATAL("No CLIP model loaded");
  if (img.type() != CV_8UC3) LFATAL("input image must be CV_8UC3 in RGB order");

  // Create a clip image from our cv::Mat with zero copy:
  clip_image_u8 const img_input
    { img.cols, img.rows, const_cast<uint8_t *>(img.data), size_t(img.rows * img.cols * 3) };

  // Pre-process image to float32 RGB with bilinear interpolation and value normalization:
  clip_image_f32 img_res;
  if (!clip_image_preprocess(itsCtx, &img_input, &img_res)) LFATAL("Failed to pre-process image for CLIP");

  // Get the embedding for the image:
  const int vec_dim = clip_get_vision_hparams(itsCtx)->projection_dim;
  cv::Mat ret(1, vec_dim, CV_32F);
  clip_image_encode(itsCtx, CLIP_THREADS, &img_res, (float *)ret.data, false);
  clip_image_f32_clean(&img_res); // NOTE: do not free img_input, its pixel data is owned by cv::Mat img
  
  // Standardize to unit norm, as expected by YOLO-JeVois:
  float const norm = cv::norm(ret);
  ret /= norm;
  
  return ret;
}

// ####################################################################################################
int jevois::dnn::CLIP::imageEmbeddingSize() const
{
  if (itsCtx == nullptr) LFATAL("No CLIP model loaded");
  if (clip_model_has_vision_encoder(itsCtx) == false) return 0;
  return clip_get_vision_hparams(itsCtx)->projection_dim;
}

// ####################################################################################################
float jevois::dnn::CLIP::similarity(cv::Mat const & emb1, cv::Mat const & emb2) const
{
  size_t const vec_dim = emb1.total();
  if (vec_dim != emb2.total()) LFATAL("Mismatched embedding sizes: " << vec_dim << " vs. " << emb2.total());
  if (emb1.type() != CV_32F || emb2.type() != CV_32F) LFATAL("Embedding type must be CV_32F");

  return clip_similarity_score((float const *)(emb1.data), (float const *)(emb2.data), vec_dim);
}

#endif // JEVOIS_PRO

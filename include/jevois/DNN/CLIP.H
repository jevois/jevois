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

#pragma once

#ifdef JEVOIS_PRO

#include <opencv2/opencv.hpp>

struct clip_ctx;

namespace jevois
{
  namespace dnn
  {
    //! Interface to a CLIP model used to compute text and image embeddings
    /*! The CLIP model runs on CPU using clip.cpp and ggml. It is used to compute text or image embeddings for
        open-world object detection models like YOLO-JeVois. The embeddings are stored in float cv::Mat with size 1x512
        for easy concatenation of several embeddings to be given as input to YOLO-JeVois as a 1xCx512 tensor for C
        object detection classes. \ingroup dnn */
    class CLIP
    {
      public:
        //! Construct and load a model from disk
        CLIP(std::string const & modelpath);
        
        //! Virtual destructor for safe inheritance
        virtual ~CLIP();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit);
 
        //! Get embedding for some text, typically as a 1x512 float matrix (depends on clip model version)
        cv::Mat textEmbedding(std::string const & txt);

        //! Get text embedding size, useful if we need to know it before getting an embedding, or 0 if no text encoder
        int textEmbeddingSize() const;

        //! Get embedding for some RGB uint8 packed image, typically as a 1x512 float matrix
        /*! Any image size is ok, the image will be rescaled and normalized to match what the CLIP model wants. */
        cv::Mat imageEmbedding(cv::Mat const & img);

        //! Get image embedding size, useful if we need to know it before getting an embedding, or 0 if no image encoder
        int imageEmbeddingSize() const;

        //! Compute cosine similarity between two embeddings
        float similarity(cv::Mat const & emb1, cv::Mat const & emb2) const;
        
      private:
        struct clip_ctx * itsCtx = nullptr; // Our clip.cpp context
    };
    
    
  } // namespace dnn
} // namespace jevois

#endif // JEVOIS_PRO

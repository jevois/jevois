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

#include <jevois/Types/PoseSkeleton.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H> // for absolutePath()
#include <opencv2/core.hpp> // for cv::fileStorage()

/*
namespace jevois
{
  static PoseSkeletonDefinition const COCO17definition
  {
    17, // number of nodes
    16, // number of links
    {   // node names
      "nose", "left eye", "right eye", "left ear", "right ear", "left shoulder", "right shoulder", "left elbow",
      "right elbow", "left wrist", "right wrist", "left hip", "right hip", "left knee", "right knee", "left ankle",
      "right ankle"
    },
    { // node colors for display, 0xAABBGGRR
      0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
      0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000
    },
    { // link definitions, each is a pair of node IDs; this is mainly for display
      {0, 1}, {1, 3}, {0, 2}, {2, 4}, {5, 6}, {5, 7}, {7, 9}, {6, 8}, {8, 10}, {5, 11}, {6, 12}, {11, 12},
      {11, 13}, {12, 14}, {13, 15}, {14, 16}
    },
    { // link colors for display, 0xAABBGGRR
      0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
      0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000
    }
  };
}
*/
  /*
const std::vector<std::vector<unsigned int>> KPS_COLORS =
        { {0,   255, 0}, {0,   255, 0},  {0,   255, 0}, {0,   255, 0},
          {0,   255, 0},  {255, 128, 0},  {255, 128, 0}, {255, 128, 0},
          {255, 128, 0},  {255, 128, 0},  {255, 128, 0}, {51,  153, 255},
          {51,  153, 255},{51,  153, 255},{51,  153, 255},{51,  153, 255},
          {51,  153, 255}};

const std::vector<std::vector<unsigned int>> SKELETON =
        { {16, 14},  {14, 12},  {17, 15},  {15, 13},   {12, 13}, {6,  12},
          {7,  13},  {6,  7},   {6,  8},   {7,  9},   {8,  10},  {9,  11},
          {2,  3}, {1,  2},  {1,  3},  {2,  4},  {3,  5},   {4,  6},  {5,  7} };

const std::vector<std::vector<unsigned int>> LIMB_COLORS =
        { {51,  153, 255}, {51,  153, 255},   {51,  153, 255},
          {51,  153, 255}, {255, 51,  255},   {255, 51,  255},
          {255, 51,  255}, {255, 128, 0},     {255, 128, 0},
          {255, 128, 0},   {255, 128, 0},     {255, 128, 0},
          {0,   255, 0},   {0,   255, 0},     {0,   255, 0},
          {0,   255, 0},   {0,   255, 0},     {0,   255, 0},
          {0,   255, 0} };
  */


// ####################################################################################################
jevois::PoseSkeletonDefinition::PoseSkeletonDefinition(std::string const & filename)
{
  std::string const fname = jevois::absolutePath(JEVOIS_SHARE_PATH, filename);
  cv::FileStorage fs(fname, cv::FileStorage::READ);
  if (fs.isOpened() == false) LFATAL("Cannot read " << fname);

  cv::FileNode node; cv::FileNodeIterator it;
  try
  {
    node = fs["nodeNames"];
    if (node.isSeq() == false) LTHROW("Required nodeNames array missing or not an array");
    for (it = node.begin(); it != node.end(); ++it)
    { cv::FileNode item = *it; nodeNames.emplace_back((std::string)(item)); }
    
    node = fs["nodeColors"];
    if (node.isSeq() == false) LTHROW("Required nodeColors array missing or not an array");
    for (it = node.begin(); it != node.end(); ++it)
    { cv::FileNode item = *it; nodeColors.emplace_back(std::stoul((std::string)(item), nullptr, 0)); }

    if (nodeNames.size() != nodeColors.size())
      LTHROW("Found " << nodeNames.size() << " node names vs. " << nodeColors.size() << " node colors");

    int nn = nodeNames.size();
    node = fs["links"];
    if (node.isSeq() == false) LTHROW("Required links array missing or not an array");
    for (it = node.begin(); it != node.end(); ++it)
    {
      cv::FileNode item = *it; cv::Point p; item >> p;
      if (p.x < 0 || p.x >= nn || p.y < 0 || p.y >= nn)
        LTHROW("Link [" << p.x << ", " << p.y <<"] node ID out of range [0 .. " << nn-1 << ']');
      links.emplace_back(std::make_pair<unsigned int, unsigned int>(p.x, p.y));
    }

    node = fs["linkColors"];
    if (node.isSeq() == false) LTHROW("Required linkColors array missing or not an array");
    for (it = node.begin(); it != node.end(); ++it)
    { cv::FileNode item = *it; linkColors.emplace_back(std::stoul((std::string)(item), nullptr, 0)); }

    if (links.size() != linkColors.size())
      LTHROW("Found " << links.size() << " links vs. " << linkColors.size() << " link colors");
  }
  catch (...)
  { LFATAL("Error parsing [" << fname << "], node [" << node.name() << "]:" << jevois::warnAndIgnoreException()); }

  LINFO("Loaded Pose Skeleton definition with " << nodeNames.size() << " nodes, " << links.size() << " links.");
}

// ####################################################################################################
jevois::PoseSkeleton::PoseSkeleton(std::shared_ptr<jevois::PoseSkeletonDefinition> def) :
    nodes(), links(), psd(def)
{
  // Internal checks for malformed definition:
  if (psd->nodeNames.size() != psd->nodeColors.size() || psd->links.size() != psd->linkColors.size())
    LFATAL("Internal error - malformed skeleton definition");
}

// ####################################################################################################
unsigned int jevois::PoseSkeleton::numSkeletonNodes() const
{ return psd->nodeNames.size(); }

// ####################################################################################################
unsigned int jevois::PoseSkeleton::numSkeletonLinks() const
{ return psd->links.size(); }

// ####################################################################################################
char const * jevois::PoseSkeleton::nodeName(unsigned int id) const
{
  if (id < psd->nodeNames.size()) return psd->nodeNames[id].c_str();
  LFATAL("Given ID " << id << " too large for skeleton  with " << psd->nodeNames.size() << " nodes");
}

// ####################################################################################################
unsigned int jevois::PoseSkeleton::nodeColor(unsigned int id) const
{
  if (id < psd->nodeNames.size()) return psd->nodeColors[id];
  LFATAL("Given ID " << id << " too large for skeleton with " << psd->nodeNames.size() << " nodes");
}

// ####################################################################################################
unsigned int jevois::PoseSkeleton::linkColor(unsigned int id) const
{
  if (id < psd->links.size()) return psd->linkColors[id];
  LFATAL("Given ID " << id << " too large for skeleton with " << psd->links.size() << " links");
}

// ####################################################################################################
std::vector<std::pair<unsigned int, unsigned int>> const & jevois::PoseSkeleton::linkDefinitions() const
{ return psd->links; }

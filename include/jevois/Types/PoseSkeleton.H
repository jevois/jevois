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

#include <jevois/Types/Enum.H>
#include <memory>

namespace jevois
{
  //! An enum for different kinds of skeletons
  /*! See PoseSkeleton.C for their definition in terms of nodes and links */
  JEVOIS_DEFINE_ENUM_CLASS(PoseSkeletonType, (Coco17))

  //! Class to hold a pose skeleton definition
  /*! \ingroup types */
  struct PoseSkeletonDefinition
  {
      //! Create from a YAML file
      PoseSkeletonDefinition(std::string const & filename);

      std::vector<std::string> nodeNames; //!< names of nodes (size nn)
      std::vector<unsigned int> nodeColors; //!< colors of nodes (size nn)
      std::vector<std::pair<unsigned int, unsigned int>> links; //!< link definitions, each is pair of node IDs (size nl)
      std::vector<unsigned int> linkColors; //!< link colors (size nl)
 };

  //! A simple skeleton class for pose detection deep neural networks
  /*! This class simply represents a set of nodes and links with associated confidences and colors. In this class, you
      should only store nodes and links that have actually been detected. \ingroup types */
  struct PoseSkeleton
  {
      //! A pose skeleton node
      struct Node
      {
          unsigned int id;  //!< Node ID (number) within the skeleton's definition of nodes
          float x, y;       //!< Node location, in image coords (b2i() already applied)
          float confidence; //!< Node confidence [0.0 .. 100.0]
      };

      //! A pose skeleton link between two nodes
      /*! In principle, from the link id we could recover the two node ids and then find those in our list of nodes to
          get their coordinates. But this is costly, so here instead we store again the coordinates of the two nodes, to
          make drawing of links fast. */
      struct Link
      {
          unsigned int id;  //!< Link id within the skeleton's definition of links
          float x1, y1;     //!< Coordinates of first joint, in image coords (b2i() already applied)
          float x2, y2;     //!< Coordinates of second joint, in image coords (b2i() already applied)
          float confidence; //!< Confidence in that link (e.g., product of the 2 node confidences), [0.0 .. 100.0]
      };

      std::vector<Node> nodes;         //!< The nodes we found in this detection
      std::vector<Link> links;         //!< The links we found in this detection

      //! Constructor from a previously loaded definition
      PoseSkeleton(std::shared_ptr<PoseSkeletonDefinition> def);

      //! Get the total number of nodes in the skeleton. Node ID is from 0 to that total - 1
      /*! This is not the number of detected nodes for a particular detection, which is simply nodes.size() */
      unsigned int numSkeletonNodes() const;
      
      //! Get the total number of links in the skeleton. Link ID is from 0 to that total - 1
      /*! This is not the number of detected links for a particular detection, which is simply links.size() */
      unsigned int numSkeletonLinks() const;
      
      //! Get the name of a node from its ID
      char const * nodeName(unsigned int id) const;

      //! Get draw color of a node from its ID, as 0xAABBGGRR (note: returned unsigned int is the same as ImU32)
      unsigned int nodeColor(unsigned int id) const;

      //! Get draw color of a node from its ID, as 0xAABBGGRR (note: returned unsigned int is the same as ImU32)
      unsigned int linkColor(unsigned int id) const;

      //! Get a const ref to our link definitions, useful to iterate over. Each link is <node_id_1, node_id_2>
      std::vector<std::pair<unsigned int, unsigned int>> const & linkDefinitions() const;

    private:
      PoseSkeleton() = delete; // Disable default constructor, we always need a skeleton definition
      std::shared_ptr<PoseSkeletonDefinition> psd;
  };

}

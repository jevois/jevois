// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2021 by Laurent Itti, the University of Southern
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

#include <jevois/DNN/Network.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Async.H>
#include <jevois/Debug/Timer.H>
#include <jevois/DNN/NetworkPython.H>

// Special output tensor number that means apply transform to all output tensors:
#define ALL_TENSORS 12345678

// ####################################################################################################
jevois::dnn::Network::~Network()
{ }

// ####################################################################################################
void jevois::dnn::Network::freeze(bool doit)
{
  comment::freeze(doit);
  url::freeze(doit);
  extraintensors::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::Network::onParamChange(network::outtransform const &, std::string const & val)
{
  itsOps.clear();
  if (val.empty()) return;

  // Split sequence by semi-colon:
  std::vector<std::string> ops = jevois::split(val, "\\s*;\\s*");

  // Decode each operation as op(arg1, arg2, ...):
  for (std::string const & op : ops)
  {
    Oper o; bool syntax_error = true;
    std::vector<std::string> tok = jevois::split(op, "(\\s*\\(\\s*|\\s*,\\s*|\\s*\\)\\s*)");
    //LINFO("op=["<<op<<"] and tok="<<jevois::join(tok, "/"));
    
    // ----------------------------------------------------------------------------------------------------
    if (tok[0] == "shape")
    {
      if (tok.size() == 3)
        try
        {
          o.op = Operator::Shape;
          o.tnum.emplace_back(std::stoul(tok[1]));
          std::vector<size_t> const newshape = jevois::dnn::strshape(tok[2]);
          for (size_t v : newshape) o.newvals.emplace_back(int(v));
          syntax_error = false;
        } catch (...) { }
      
      if (syntax_error) LFATAL("Syntax error, expected: shape(outnum, AxBxC...)");
    }
    // ----------------------------------------------------------------------------------------------------
    else if (tok[0] == "transpose")
    {
      if (tok.size() >= 3)
        try
        {
          o.op = Operator::Transpose;
          if (tok[1] == "*") o.tnum.emplace_back(ALL_TENSORS); // Special tensor number * means all of them
          else o.tnum.emplace_back(std::stoul(tok[1]));
          for (size_t i = 2; i < tok.size(); ++i) o.newvals.emplace_back(int(std::stoul(tok[i])));
          syntax_error = false;
        } catch (...) { }
      
      if (syntax_error) LFATAL("Syntax error, expected: transpose(outnum, oldaxisA, oldaxisB, ...), where "
                               "transposed new axis 0 (the outermost dimension, typically batch size) will be "
                               "from oldaxisA, new axis 1 from oldaxisB, etc");
    }
    // ----------------------------------------------------------------------------------------------------
    else if (tok[0] == "order")
    {
      if (tok.size() >= 3)
        try
        {
          o.op = Operator::Order;
          for (size_t i = 1; i < tok.size(); ++i) o.newvals.emplace_back(int(std::stoul(tok[i])));
          syntax_error = false;
        } catch (...) { }
      
      if (syntax_error) LFATAL("Syntax error, expected: order(oldidx0, oldidx1, ...), where the new order will be "
                               "new tensor 0: old tensor oldidx0 (which is zero-based); new tensor 1: "
                               "old tensor oldidx1, etc. It is ok to have duplicated or missing entries.");
    }
    // ----------------------------------------------------------------------------------------------------
    else if (tok[0] == "split")
    {
      if (tok.size() >= 4)
        try
        {
          o.op = Operator::Split;
          if (tok[1] == "*") o.tnum.emplace_back(ALL_TENSORS); // Special tensor number * means all of them
          else o.tnum.emplace_back(std::stoul(tok[1]));
          o.tnum.emplace_back(std::stoul(tok[2])); // axis number
          for (size_t i = 3; i < tok.size(); ++i) o.newvals.emplace_back(int(std::stoul(tok[i])));
          syntax_error = false;
        } catch (...) { }
      
      if (syntax_error) LFATAL("Syntax error, expected: split(outnum, axis, newsize1, ..., newsizeN), where "
                               "axis 0 is the outtermost dimension (typically, batch size), and newsize1 + ... "
                               "+ newsizeN must be equal to the original size of that axis.");
    }
    // ----------------------------------------------------------------------------------------------------
    else if (tok[0] == "merge")
    {
      if (tok.size() >= 3)
        try
        {
          o.op = Operator::Merge;
          o.tnum.emplace_back(std::stoul(tok[1])); // axis number
          for (size_t i = 2; i < tok.size(); ++i) o.newvals.emplace_back(int(std::stoul(tok[i])));
          syntax_error = false;

          // Check that tensors to merge are listed in ascending order:
          for (size_t i = 0; i < o.newvals.size() - 1; ++i)
            if (o.newvals[i] > o.newvals[i+1]) { syntax_error = true; break; }
        } catch (...) { }
      
      if (syntax_error) LFATAL("Syntax error, expected: merge(axis, outnum1, ..., outnumN), where "
                               "axis 0 is the outermost dimension (typically, batch size) and outnum1, ..., "
                               "outnumN are the outputs to merge along that axis. All the outputs to be merged "
                               "must have matching number of dimensions, and matching sizes on all other axes. "
                               "The merged tensor will replace the first output listed in the merge, and the other "
                               "listed will be removed. Outputs to merge must be listed in ascending order (use "
                               "an order() transform first if needed)");
    }
    // ----------------------------------------------------------------------------------------------------
     else LFATAL("Syntax error: Unrecognized operation: " << op);

    itsOps.emplace_back(o);
  }
}
  
// ####################################################################################################
void jevois::dnn::Network::waitBeforeDestroy()
{
  // Do not destroy a network that is loading, and do not throw...
  size_t count = 0;
  while (itsLoading.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    try { if (ready()) break; } catch (...) { }
    if (count++ == 200) { LINFO("Waiting for network load to complete..."); count = 0; }
  }
}

// ####################################################################################################
bool jevois::dnn::Network::ready()
{
  // If we are loaded, we are ready to process:
  if (itsLoaded.load()) return true;
  
  // If we are loading, check whether loading is complete or threw, otherwise return false as we keep loading:
  if (itsLoading.load())
  {
    if (itsLoadFut.valid() && itsLoadFut.wait_for(std::chrono::milliseconds(2)) == std::future_status::ready)
    {
      try { itsLoadFut.get(); itsLoaded.store(true); itsLoading.store(false); LINFO("Network loaded."); return true; }
      catch (...) { itsLoading.store(false); jevois::warnAndRethrowException(); }
    }
    return false;
  }

  // For Python networks, we need to load in the current thread and block everyone...
  if (dynamic_cast<jevois::dnn::NetworkPython *>(this) != nullptr)
  {
    LINFO("Loading network...");
    this->load();
    itsLoaded.store(true);
    itsLoading.store(false);
    LINFO("Network loaded.");
    return true;
  }
  
  // Otherwise, trigger an async load:
  itsLoading.store(true);
  itsLoadFut = jevois::async(std::bind(&jevois::dnn::Network::load, this));
  LINFO("Loading network...");

  return false;
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::Network::process(std::vector<cv::Mat> const & blobs,
                                                   std::vector<std::string> & info)
{
  if (ready() == false) LFATAL("Network is not ready");
  static jevois::TimerOne eitimer("Create extra inputs");
  static jevois::TimerOne tftimer("Transform outputs");

  std::vector<cv::Mat> outs;
  std::string const c = comment::get();
  
  // Add any extra input tensors?
  std::string const extra = extraintensors::get();
  if (extra.empty() == false)
  {
    eitimer.start();
    
    std::vector<cv::Mat> newblobs = blobs;
      
    std::vector<std::string> ins = jevois::split(extra, ",\\s*");
    for (std::string const & in : ins)
    {
      vsi_nn_tensor_attr_t attr; memset(&attr, 0, sizeof(attr));

      std::vector<std::string> tok = jevois::split(in, ":");
      if (tok.size() != 3)
        LFATAL("Malformed extra tensor, need <type>:<shape>:val1 val2 ... valN (separate multiple tensors by comma)");

      // Decode type and convert to vsi, only those types that OpenCV can support:
      if (tok[0] == "8U") attr.dtype.vx_type = VSI_NN_TYPE_UINT8;
      else if (tok[0] == "8S") attr.dtype.vx_type = VSI_NN_TYPE_INT8;
      else if (tok[0] == "16U") attr.dtype.vx_type = VSI_NN_TYPE_UINT16;
      else if (tok[0] == "16S") attr.dtype.vx_type = VSI_NN_TYPE_INT16;
      else if (tok[0] == "16F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT16;
      else if (tok[0] == "32S") attr.dtype.vx_type = VSI_NN_TYPE_INT32;
      else if (tok[0] == "32F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT32; 
      else if (tok[0] == "64F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT64; 
      else throw std::range_error("Unsupported extra input tensor type [" + tok[0] + "] in " + extra);

      // Decode the dims:
      std::vector<size_t> dims = jevois::dnn::strshape(tok[1]);
      attr.dim_num = dims.size();
      for (size_t i = 0; i < attr.dim_num; ++i) attr.size[attr.dim_num - 1 - i] = dims[i];

      // Allocate the tensor:
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
      attr.dtype.fmt = VSI_NN_DIM_FMT_AUTO;
      cv::Mat b = jevois::dnn::attrmat(attr);

      // Populate the values:
      std::vector<std::string> vals = jevois::split(tok[2], "\\s+");
      size_t const nvals = vals.size();
      if (nvals != b.total())
        LFATAL("Extra in tensor needs " << b.total() << " values, but " << nvals << " given in [" << in << ']');
      switch (attr.dtype.vx_type)
      {
      case VSI_NN_TYPE_UINT8:
      {
        uint8_t * ptr = reinterpret_cast<uint8_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;

      case VSI_NN_TYPE_INT8:
      {
        int8_t * ptr = reinterpret_cast<int8_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;
      
      case VSI_NN_TYPE_UINT16:
      {
        uint16_t * ptr = reinterpret_cast<uint16_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;

      case VSI_NN_TYPE_INT16:
      {
        int16_t * ptr = reinterpret_cast<int16_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;
      
      case VSI_NN_TYPE_FLOAT16:
      {
        cv::hfloat * ptr = reinterpret_cast<cv::hfloat *>(b.data);
        for (std::string const & v : vals) *ptr++ = cv::hfloat(std::stof(v));
      }
      break;

      case VSI_NN_TYPE_INT32:
      {
        int32_t * ptr = reinterpret_cast<int32_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;

      case VSI_NN_TYPE_FLOAT32:
      {
        float * ptr = reinterpret_cast<float *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stof(v);
      }
      break;

      case VSI_NN_TYPE_FLOAT64:
      {
        double * ptr = reinterpret_cast<double *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stod(v);
      }
      break;
      
      default: LFATAL("internal inconsistency");
      }
      
      newblobs.emplace_back(std::move(b));
    }

    // NOTE: Keep the code below in sync with the default case (no extra inputs). Both branches are duplicated to avoid
    // having to make a copy of blobs into newblobs in the standard case when we do not have any extra inputs:
    
    // Show info about input tensors:
    info.emplace_back("* Input Tensors");
    for (cv::Mat const & b : newblobs) info.emplace_back("- " + jevois::dnn::shapestr(b));
    info.emplace_back(eitimer.stop());

    // Run processing on the derived class:
    info.emplace_back("* Network");
    if (c.empty() == false) info.emplace_back(c);
  
    outs = doprocess(newblobs, info);
  }
  else
  {
    // Show info about input tensors:
    info.emplace_back("* Input Tensors");
    for (cv::Mat const & b : blobs) info.emplace_back("- " + jevois::dnn::shapestr(b));
    
    // Run processing on the derived class:
    info.emplace_back("* Network");
    if (c.empty() == false) info.emplace_back(c);
    
    outs = doprocess(blobs, info);
  }
    
  // Show info about output tensors:
  info.emplace_back("* Output Tensors");
  for (size_t i = 0; i < outs.size(); ++i) info.emplace_back("- " + jevois::dnn::shapestr(outs[i]));

  // Possibly apply some transformation sequence to the outputs:
  if (itsOps.empty() == false)
  {
    tftimer.start();
    
    info.emplace_back("* Output Tensors Transforms");

    for (Oper const & o : itsOps)
      switch(o.op)
      {
        // ----------------------------------------------------------------------------------------------------
      case Operator::Shape:
      {
        // tnum guaranteed to have 1 entry; newvals has a valid nD shape.
        size_t const tnum = o.tnum[0];

        try
        {
          outs[tnum] = outs[tnum].reshape(1, o.newvals);
          info.emplace_back("- shape out " + std::to_string(tnum) + " to " + jevois::dnn::shapestr(outs[tnum]));
        }
        catch (...)
        {
          LFATAL("While attempting output transform 'shape(" << tnum << ", " <<
                 jevois::dnn::shapestr(o.newvals, outs[tnum].type()) << ")': Cannot reshape from " <<
                 jevois::dnn::shapestr(outs[tnum]) << " to desired dims because of total number of elements mismatch");
        }
      }
      break;
      
        // ----------------------------------------------------------------------------------------------------
      case Operator::Transpose:
      {
        // tnum guaranteed to have 1 entry; newvals has a list of axis numbers
        size_t tnum = o.tnum[0];

        // Possibly parallelize if more than one transpose to do:
        if (tnum == ALL_TENSORS)
        {
          std::vector<std::future<void>> fvec;
          for (size_t t = 0; t < outs.size(); ++t)
          {
            info.emplace_back("- transpose out " + std::to_string(t) + " to " + jevois::dnn::shapestr(outs[t]));
            fvec.emplace_back(jevois::async([&](size_t t)
            {
              try
              {
                // Do the transpose. cv::transposeND() needs separate source and dest tensors:
                cv::Mat newout; cv::transposeND(outs[t], o.newvals, newout); outs[t] = std::move(newout);
              }
              catch (...)
              {
                LFATAL("While attempting output transform 'transpose(" << t << ", " << jevois::join(o.newvals, ", ") <<
                       ")': Cannot transpose from " << jevois::dnn::shapestr(outs[t]) <<
                       " to desired shape, check number of dimensions and that the desired axes contain every "
                       "source axis number exactly once.");
              }
            }, t));
          }

          // Use joinall() to get() all futures and throw a single consolidated exception if any thread threw:
          jevois::joinall(fvec);
        }
        else
        {
          // Only one tensor to transpose:
          try
          {
            // Do the transpose. cv::transposeND() needs separate source and dest tensors:
            cv::Mat newout; cv::transposeND(outs[tnum], o.newvals, newout); outs[tnum] = std::move(newout);
            info.emplace_back("- transpose out " + std::to_string(tnum) + " to " + jevois::dnn::shapestr(outs[tnum]));
          }
          catch (...)
          {
            LFATAL("While attempting output transform 'transpose(" << tnum << ", " << jevois::join(o.newvals, ", ") <<
                   ")': Cannot transpose from " << jevois::dnn::shapestr(outs[tnum]) <<
                   " to desired shape, check number of dimensions and that the desired axes contain every source axis "
                   "number exactly once.");
          }
        }
      }
      break;

      // ----------------------------------------------------------------------------------------------------
      case Operator::Order:
      {
        std::vector<cv::Mat> newouts; int const osiz = int(outs.size());

        for (int idx : o.newvals)
          if (idx >= 0 && idx < osiz)
            newouts.push_back(outs[idx]); // no emplace_back() here as one tensor may be pushed several times
          else
            LFATAL("While attempting output transform 'order(" << jevois::join(o.newvals, ", ") <<
                   ")': Output number " << idx << " does not exist");

        info.emplace_back("- order: " + jevois::join(o.newvals, ", "));
        outs = std::move(newouts);
      }
      break;

      // ----------------------------------------------------------------------------------------------------
      case Operator::Split:
      {
        size_t const tnum = o.tnum[0];
        size_t const axis = o.tnum[1];
        
        std::vector<cv::Mat> newouts;
        for (size_t i = 0; i < outs.size(); ++i)
          if (i == tnum || tnum == ALL_TENSORS)
          {
            // Split that tensor and push the resulting tensors. split() will check validity of axis, sizes, etc:
            try
            {
              std::vector<cv::Mat> mats = jevois::dnn::split(outs[i], axis, o.newvals);

              // Add those mats, create info string:
              std::string inf = "- split out " + std::to_string(i) + " to ";
              for (cv::Mat & m : mats)
              {
                inf += jevois::dnn::shapestr(m) + ", ";
                newouts.emplace_back(m);
              }
              info.emplace_back(inf.substr(0, inf.length() - 2));
            }
            catch (std::exception const & e)
            {
              LFATAL("While attempting output transform 'split(" << i << ", " << axis << ", " <<
                     jevois::join(o.newvals, ", ") << ")': error: " << e.what());
            }
            catch (...)
            {
              LFATAL("While attempting output transform 'split(" << i << ", " << axis << ", " <<
                     jevois::join(o.newvals, ", ") << ")': unknown error");
            }
          }
          else newouts.push_back(outs[i]); // Just transfer that tensor

        outs = std::move(newouts);
      }
      break;

      // ----------------------------------------------------------------------------------------------------
      case Operator::Merge:
      {
        size_t const axis = o.tnum[0]; size_t const numouts = outs.size();
        std::vector<cv::Mat> newouts;

        // Decide what to do for each output: 0=copy over, 1=run the merge and store result, 2=skip other merge parts.
        // Also build a vector of those tensors to merge:
        int action[numouts]; bool didmerge = false; std::vector<cv::Mat> tomerge;
        for (int i = 0; i < int(numouts); ++i)
        {
          if (outs[i].type() != CV_32F && outs[i].type() != CV_64F && outs[i].type() != CV_16F)
            LFATAL("While attempting output transform 'merge(" << axis << ", " << jevois::join(o.newvals, ", ") <<
                   ")': Cannot merge quantized tensors");

          // Check if i is in our merge list:
          bool inmerge = false; for (int j : o.newvals) if (i == j) { inmerge = true; break; }
          if (inmerge)
          {
            tomerge.push_back(outs[i]); // no emplace_back() here as we might want to duplicate a tensor
            if (didmerge == false) { action[i] = 1; didmerge = true; } else action[i] = 2;
          }
          else action[i] = 0;
        }

        // Ready to rock:
        for (size_t i = 0; i < numouts; ++i)
          try
          {
            switch (action[i])
            {
            case 0: // push that tensor unmodified
              newouts.push_back(outs[i]);
              break;

            case 1: // push the merged tensor
              newouts.emplace_back(jevois::dnn::concatenate(tomerge, axis));
              info.emplace_back("- merged outs " + jevois::join(o.newvals, ", ") + " into " +
                                jevois::dnn::shapestr(newouts.back()) + " (new out " +
                                std::to_string(newouts.size()-1) + ')');
              break;

            case 2: // skip the other tensors that were merged
              break;

            default: LFATAL("Internal inconsistency in merge() transform");
            }
          }
          catch (std::exception const & e)
          {
            LFATAL("While attempting output transform 'merge(" << axis << ", " <<
                   jevois::join(o.newvals, ", ") << ")': error: " << e.what());
          }
          catch (...)
          {
            LFATAL("While attempting output transform 'merge(" << axis << ", " <<
                   jevois::join(o.newvals, ", ") << ")': unknown error");
          }
        
        outs = std::move(newouts);
      }
      break;

      // ----------------------------------------------------------------------------------------------------
      default:
        LFATAL("Internal error: Unsupported output transform op " << int(o.op));
      }

    info.emplace_back(tftimer.stop());

    // Show info about transformed output tensors:
    info.emplace_back("* Transformed Output Tensors");
    for (size_t i = 0; i < outs.size(); ++i) info.emplace_back("- " + jevois::dnn::shapestr(outs[i]));
  }
  
  return outs;
}

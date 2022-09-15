// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>
#include <fstream>

// ##############################################################################################################
std::map<int, std::string> jevois::dnn::readLabelsFile(std::string const & fname)
{
  std::ifstream ifs(fname);
  if (ifs.is_open() == false) LFATAL("Failed to open file " << fname);

  size_t linenum = 1; std::map<int, std::string> ret; int id = 0;
  for (std::string line; std::getline(ifs, line); ++linenum)
  {
    size_t idx1 = line.find_first_not_of(" \t"); if (idx1 == line.npos) continue;
    size_t idx2 = line.find_last_not_of(" \t\r\n"); if (idx2 == line.npos) continue;
    if (line[idx1] == '#') continue;

    try { id = std::stoi(line, &idx1); idx1 = line.find_first_not_of("0123456789 \t,:", idx1); } catch (...) { }

    std::string classname;
    if (idx1 >= idx2)
    {
      LERROR(fname << ':' << linenum << ": empty class name -- REPLACING BY 'unspecified'");
      classname = "unspecified";
    }
    else classname = line.substr(idx1, idx2 + 1 - idx1);

    // Possibly replace two double quotes by one:
    jevois::replaceStringAll(classname, "\"\"", "\"");

    // Possibly remove enclosing double quotes:
    size_t len = classname.length();
    if (len > 1 && classname[0] == '"' && classname[len-1] == '"') classname = classname.substr(1, len-2);
    
    ret[id] = classname;
    
    // Increment id in case no ID number is given in the file:
    ++id;
  }

  ifs.close();

  LINFO("Loaded " << ret.size() << " class names from " << fname);
  
  return ret;
}

// ##############################################################################################################
std::string jevois::dnn::getLabel(std::map<int, std::string> const & labels, int id)
{
  auto itr = labels.find(id);
  if (itr == labels.end()) return std::to_string(id);
  return itr->second;
}

// ##############################################################################################################
int jevois::dnn::stringToRGBA(std::string const & label, unsigned char alpha)
{
  int col = 0x80808080;
  for (char const c : label) col = c + ((col << 5) - col);
  col = (col & 0xffffff) | (alpha << 24);
  return col;
}

// ##############################################################################################################
void jevois::dnn::topK(float const * pfProb, float * pfMaxProb, uint32_t * pMaxClass, uint32_t outputCount,
                       uint32_t topNum)
{
  memset(pfMaxProb, 0xfe, sizeof(float) * topNum);
  memset(pMaxClass, 0xff, sizeof(float) * topNum);
  
  for (uint32_t j = 0; j < topNum; ++j)
  {
    for (uint32_t i = 0; i < outputCount; ++i)
    {
      uint32_t k;
      for (k = 0; k < topNum; ++k) if (i == pMaxClass[k]) break;
      if (k != topNum) continue;
      
      if (pfProb[i] > pfMaxProb[j]) { pfMaxProb[j] = pfProb[i]; pMaxClass[j] = i; }
    }
  }
}

// ##############################################################################################################
std::string jevois::dnn::shapestr(cv::Mat const & m)
{
  cv::MatSize const & ms = m.size; int const nd = ms.dims();
  std::string ret = std::to_string(nd) + "D ";
  for (int i = 0; i < nd; ++i) ret += std::to_string(ms[i]) + (i < nd-1 ? "x" : "");
  ret += ' ' + jevois::cvtypestr(m.type());
  return ret;
}

// ##############################################################################################################
std::string jevois::dnn::shapestr(TfLiteTensor const * t)
{

  TfLiteIntArray const & dims = *t->dims;
  std::string ret = std::to_string(dims.size) + "D ";
  for (int i = 0; i < dims.size; ++i) ret += std::to_string(dims.data[i]) + (i < dims.size-1 ? "x" : "");

  // Do not use TfLiteTypeGetName() as it returns different names...
  switch (t->type)
  { 
  case kTfLiteNoType: ret += " NoType"; break;
  case kTfLiteFloat32: ret += " 32F"; break;
  case kTfLiteInt32: ret += " 32S"; break;
  case kTfLiteUInt8: ret += " 8U"; break;
  case kTfLiteInt64: ret += " 64S"; break;
  case kTfLiteString: ret += " String"; break;
  case kTfLiteBool: ret += " 8B"; break;
  case kTfLiteInt16: ret += " 16S"; break;
  case kTfLiteComplex64: ret += " 64C"; break;
  case kTfLiteInt8: ret += " 8I"; break;
  case kTfLiteFloat16: ret += " 16F"; break;
  case kTfLiteFloat64: ret += " 64F"; break;
  case kTfLiteComplex128: ret += " 128C"; break;
  default: ret += " UnknownType"; break;
  }
  return ret;
}

// ##############################################################################################################
std::string jevois::dnn::shapestr(vsi_nn_tensor_attr_t const & attr)
{
  std::string ret = std::to_string(attr.dim_num) + "D ";
  for (uint32_t i = 0; i < attr.dim_num; ++i)
    ret += std::to_string(attr.size[attr.dim_num-1-i]) + (i < attr.dim_num-1 ? "x" : "");

  // Value type:
  switch (attr.dtype.vx_type)
  {
  case VSI_NN_TYPE_UINT8: ret += " 8U"; break;
  case VSI_NN_TYPE_INT8: ret += " 8S"; break;
  case VSI_NN_TYPE_BOOL8: ret += " 8B"; break;
  case VSI_NN_TYPE_UINT16: ret += " 16U"; break;
  case VSI_NN_TYPE_INT16: ret += " 16S"; break;
  case VSI_NN_TYPE_FLOAT16: ret += " 16F"; break;
  case VSI_NN_TYPE_BFLOAT16: ret += " 16B"; break;
  case VSI_NN_TYPE_UINT32: ret += " 32U"; break;
  case VSI_NN_TYPE_INT32: ret += " 32S"; break;
  case VSI_NN_TYPE_FLOAT32: ret += " 32F"; break;
  case VSI_NN_TYPE_UINT64: ret += " 64U"; break;
  case VSI_NN_TYPE_INT64: ret += " 64S"; break;
  case VSI_NN_TYPE_FLOAT64: ret += " 64F"; break;
  default: throw std::range_error("shapestr: Unsupported tensor type " + std::to_string(attr.dtype.vx_type));
  }

  return ret;
}

// ##############################################################################################################
#ifdef JEVOIS_PRO
std::string jevois::dnn::shapestr(hailo_vstream_info_t const & vi)
{
  // FIXME: should optimize but beware that vi.shape may have different interpretations depending on NCHW vs NWCH etc
  return shapestr(tensorattr(vi));

  /*
  std::string ret = "3D ";
  switch (+ std::to_string(vi.shape[0]) + 'x' +
    std::to_string(vi.shape[1]) + 'x' + std::to_string(vi.shape[2]);

  switch (vi.format.type)
  {
  case HAILO_FORMAT_TYPE_AUTO: ret += " AUTO"; break;
  case HAILO_FORMAT_TYPE_UINT8: ret += " 8U"; break;
  case HAILO_FORMAT_TYPE_UINT16: ret += " 16U"; break;
  case HAILO_FORMAT_TYPE_FLOAT32: ret += " 32F"; break;
  default: throw std::range_error("shapestr: Unsupported tensor type " + std::to_string(vi.format));
  }

  return ret;
  */
}
#endif

// ##############################################################################################################
std::vector<size_t> jevois::dnn::strshape(std::string const & str)
{
  std::vector<size_t> ret;
  auto tok = jevois::split(str, "x");
  for (std::string const & t : tok) ret.emplace_back(std::stoi(t));
  return ret;
}

// ##############################################################################################################
int jevois::dnn::tf2cv(TfLiteType t)
{
  switch (t)
  {
  case kTfLiteFloat32: return CV_32F;
  case kTfLiteInt32: return CV_32S;
  case kTfLiteUInt8: return CV_8U;
  case kTfLiteInt16: return CV_16S;
  case kTfLiteInt8: return CV_8S;
  case kTfLiteFloat16: return CV_16S;
  case kTfLiteFloat64: return CV_64F;
    //case kTfLiteComplex128:
    //case kTfLiteComplex64:
    //case kTfLiteBool:
    //case kTfLiteString:
    //case kTfLiteInt64:
    //case kTfLiteNoType:
  default: throw std::range_error(std::string("tf2cv: Unsupported type ") + TfLiteTypeGetName(t));
  }
}

// ##############################################################################################################
int jevois::dnn::vsi2cv(vsi_nn_type_e t)
{
  switch (t)
  {
  case VSI_NN_TYPE_UINT8: return CV_8U;
  case VSI_NN_TYPE_INT8: return CV_8S;
  case VSI_NN_TYPE_BOOL8: return CV_8U;
  case VSI_NN_TYPE_UINT16: return CV_16U;
  case VSI_NN_TYPE_INT16: return CV_16S;
  case VSI_NN_TYPE_FLOAT16: return CV_16F;
  case VSI_NN_TYPE_BFLOAT16: return CV_16F; // check
    //case VSI_NN_TYPE_UINT32: return CV_32U; // unsupported by opencv
  case VSI_NN_TYPE_INT32: return CV_32S;
  case VSI_NN_TYPE_FLOAT32: return CV_32F;
    //case VSI_NN_TYPE_UINT64: return CV_64U; // unsupported by opencv
    //case VSI_NN_TYPE_INT64: return CV_64S; // unsupported by opencv
  case VSI_NN_TYPE_FLOAT64: return CV_64F;
  default: throw std::range_error("vsi2cv: Unsupported tensor type " + std::to_string(t));
  }
}

// ##############################################################################################################
vsi_nn_type_e jevois::dnn::tf2vsi(TfLiteType t)
{
  switch (t)
  {
  case kTfLiteFloat32: return VSI_NN_TYPE_FLOAT32;
  case kTfLiteInt32: return VSI_NN_TYPE_INT32;
  case kTfLiteUInt8: return VSI_NN_TYPE_UINT8;
  case kTfLiteInt16: return VSI_NN_TYPE_INT16;
  case kTfLiteInt8: return VSI_NN_TYPE_INT8;
  case kTfLiteFloat16: return VSI_NN_TYPE_FLOAT16;
  case kTfLiteFloat64: return VSI_NN_TYPE_FLOAT64;
  case kTfLiteInt64: return VSI_NN_TYPE_INT64;
  case kTfLiteBool: return VSI_NN_TYPE_BOOL8; // fixme: need to check
  case kTfLiteNoType: return VSI_NN_TYPE_NONE;
    //case kTfLiteComplex128:
    //case kTfLiteComplex64:
    //case kTfLiteString:
  default: throw std::range_error(std::string("tf2vsi: Unsupported type ") + TfLiteTypeGetName(t));
  }
}

// ##############################################################################################################
#ifdef JEVOIS_PRO
vsi_nn_type_e jevois::dnn::hailo2vsi(hailo_format_type_t t)
{
  switch (t)
  {
  case HAILO_FORMAT_TYPE_AUTO: return VSI_NN_TYPE_NONE; break; // or throw?
  case HAILO_FORMAT_TYPE_UINT8: return VSI_NN_TYPE_UINT8; break;
  case HAILO_FORMAT_TYPE_UINT16: return VSI_NN_TYPE_UINT16; break;
  case HAILO_FORMAT_TYPE_FLOAT32: return VSI_NN_TYPE_FLOAT32; break;
  default: throw std::range_error("hailo2vsi: Unsupported tensor type " + std::to_string(t));
  }
}
#endif

// ##############################################################################################################
void jevois::dnn::clamp(cv::Rect & r, int width, int height)
{
  int tx = std::min(width - 1, std::max(0, r.x));
  int ty = std::min(height - 1, std::max(0, r.y));
  int bx = std::min(width - 1, std::max(0, r.x + r.width));
  int by = std::min(height - 1, std::max(0, r.y + r.height));
  r.x = tx; r.y = ty; r.width = bx - tx; r.height = by - ty;
}

// ##############################################################################################################
void jevois::dnn::clamp(cv::Rect2f & r, float width, float height)
{
  float tx = std::min(width - 1.0F, std::max(0.0F, r.x));
  float ty = std::min(height - 1.0F, std::max(0.0F, r.y));
  float bx = std::min(width - 1.0F, std::max(0.0F, r.x + r.width));
  float by = std::min(height - 1.0F, std::max(0.0F, r.y + r.height));
  r.x = tx; r.y = ty; r.width = bx - tx; r.height = by - ty;
}

// ##############################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::parseTensorSpecs(std::string const & specs)
{
  char const * const specdef = "[NCHW:|NHWC:|NA:|AUTO:]Type:[NxCxHxW|NxHxWxC|...][:QNT[:fl|:scale:zero]]";
  std::vector<std::string> spectok = jevois::split(specs, ",\\s*");
  std::vector<vsi_nn_tensor_attr_t> ret;
  
  for (std::string const & spec : spectok)
  {
    vsi_nn_tensor_attr_t attr; memset(&attr, 0, sizeof(attr));

    // NCHW:Type:NxCxHxW:QNT:scale:mean
    std::vector<std::string> tok = jevois::split(spec, ":");
    if (tok.size() < 2) throw std::runtime_error("parseTensorSpecs: Malformed tensor spec ["+spec+"] not "+specdef);
    
    // Decode optional shape:
    size_t n = 0; // next tok to parse
    if (tok[0] == "NCHW") { ++n; attr.dtype.fmt = VSI_NN_DIM_FMT_NCHW; } // planar RGB
    else if (tok[0] == "NHWC") { ++n; attr.dtype.fmt = VSI_NN_DIM_FMT_NHWC; } // packed RGB
    else if (tok[0] == "NA") { ++n; attr.dtype.fmt = VSI_NN_DIM_FMT_NA; }
    else if (tok[0] == "AUTO") { ++n; attr.dtype.fmt = VSI_NN_DIM_FMT_AUTO; }
    else attr.dtype.fmt = VSI_NN_DIM_FMT_AUTO; // use AUTO if it was not given

    // We need at least type and dims:
    if (tok.size() < n+2) throw std::runtime_error("parseTensorSpecs: Malformed tensor spec ["+spec+"] not "+specdef);

    // Decode type and convert to vsi:
    if (tok[n] == "8U") attr.dtype.vx_type = VSI_NN_TYPE_UINT8;
    else if (tok[n] == "8S") attr.dtype.vx_type = VSI_NN_TYPE_INT8;
    else if (tok[n] == "8B") attr.dtype.vx_type = VSI_NN_TYPE_BOOL8;
    else if (tok[n] == "16U") attr.dtype.vx_type = VSI_NN_TYPE_UINT16;
    else if (tok[n] == "16S") attr.dtype.vx_type = VSI_NN_TYPE_INT16;
    else if (tok[n] == "16F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT16;
    else if (tok[n] == "16B") attr.dtype.vx_type = VSI_NN_TYPE_BFLOAT16;
    else if (tok[n] == "32U") attr.dtype.vx_type = VSI_NN_TYPE_UINT32;
    else if (tok[n] == "32S") attr.dtype.vx_type = VSI_NN_TYPE_INT32;
    else if (tok[n] == "32F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT32; 
    else if (tok[n] == "64U") attr.dtype.vx_type = VSI_NN_TYPE_UINT64;
    else if (tok[n] == "64S") attr.dtype.vx_type = VSI_NN_TYPE_INT64;
    else if (tok[n] == "64F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT64; 
    else throw std::range_error("parseTensorSpecs: Invalid tensor type [" + tok[n] + "] in " + spec);
    ++n; // next token
    
    // Decode the dims:
    std::vector<size_t> dims = jevois::dnn::strshape(tok[n]);
    attr.dim_num = dims.size();
    for (size_t i = 0; i < attr.dim_num; ++i) attr.size[attr.dim_num - 1 - i] = dims[i];
    ++n; // next token
    
    // Decode optional quantization type and its possible extra parameters:
    if (n == tok.size() || tok[n] == "NONE")
    {
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
    }
    else if (tok[n] == "DFP")
    {
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
      if (tok.size() != n+2)
        throw std::range_error("parseTensorSpecs: In "+spec+", DFP quantization needs :fl (" + specdef + ')');
      attr.dtype.fl = std::stoi(tok[n+1]);
    }
    
    else if (tok[n] == "AA" || tok[n] == "AS") // affine asymmetric and symmetric same, see ovxlib/vsi_nn_tensor.h
    {
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC;
      if (tok.size() != n+3)
        throw std::range_error("parseTensorSpecs: In "+spec+", AA/AS quantization needs :scale:zero ("+specdef+')');
      attr.dtype.scale = std::stof(tok[n+1]);
      attr.dtype.zero_point = std::stoi(tok[n+2]);
    }
    else if (tok[n] == "APS")
    {
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_AFFINE_PERCHANNEL_SYMMETRIC;
      throw std::range_error("parseTensorSpecs: In " + spec + ", AFFINE_PERCHANNEL_SYMMETRIC quant not yet supported");
    }
    else throw std::range_error("parseTensorSpecs: Invalid quantization type in " + spec);

    // Done with this tensor:
    ret.emplace_back(attr);
  }

  return ret;
}

// ##############################################################################################################
cv::Mat jevois::dnn::attrmat(vsi_nn_tensor_attr_t const & attr, void * dataptr)
{
  if (dataptr) return cv::Mat(jevois::dnn::attrdims(attr), jevois::dnn::vsi2cv(attr.dtype.vx_type), dataptr);
  else return cv::Mat(jevois::dnn::attrdims(attr), jevois::dnn::vsi2cv(attr.dtype.vx_type));
}

// ##############################################################################################################
std::vector<int> jevois::dnn::attrdims(vsi_nn_tensor_attr_t const & attr)
{
  size_t const ndim = attr.dim_num;
  std::vector<int> cvdims(ndim);
  for (size_t i = 0; i < ndim; ++i) cvdims[ndim - 1 - i] = attr.size[i];
  return cvdims;
}

// ##############################################################################################################
cv::Size jevois::dnn::attrsize(vsi_nn_tensor_attr_t const & attr)
{
  switch (attr.dtype.fmt)
  {
  case VSI_NN_DIM_FMT_NHWC:
    if (attr.dim_num < 3) throw std::range_error("attrsize: need at least 3D, got " + jevois::dnn::attrstr(attr));
    return cv::Size(attr.size[1], attr.size[2]);
    
  case VSI_NN_DIM_FMT_NCHW:
    if (attr.dim_num < 2) throw std::range_error("attrsize: need at least 2D, got " + jevois::dnn::attrstr(attr));
    return cv::Size(attr.size[0], attr.size[1]);

  case VSI_NN_DIM_FMT_AUTO:
    if (attr.dim_num < 2) throw std::range_error("attrsize: need at least 2D, got " + jevois::dnn::attrstr(attr));
    if (attr.dim_num < 3) return cv::Size(attr.size[0], attr.size[1]);
    // ok, size[] starts with either CWH (when dim index goes 0..2) or WHC, assume C<H
    if (attr.size[0] > attr.size[2]) return cv::Size(attr.size[0], attr.size[1]); // WHCN
    else return cv::Size(attr.size[1], attr.size[2]); // CWHN
    
  default:
    throw std::range_error("attrsize: cannot extract width and height, got " + jevois::dnn::attrstr(attr));
  }
}

// ##############################################################################################################
std::string jevois::dnn::attrstr(vsi_nn_tensor_attr_t const & attr)
{
  std::string ret;

  // Dimension ordering:
  switch (attr.dtype.fmt)
  {
  case VSI_NN_DIM_FMT_NCHW: ret += "NCHW:"; break;
  case VSI_NN_DIM_FMT_NHWC: ret += "NHWC:"; break;
  default: break;
  }

  // Value type:
  switch (attr.dtype.vx_type)
  {
  case VSI_NN_TYPE_UINT8: ret += "8U:"; break;
  case VSI_NN_TYPE_INT8: ret += "8S:"; break;
  case VSI_NN_TYPE_BOOL8: ret += "8B:"; break;
  case VSI_NN_TYPE_UINT16: ret += "16U:"; break;
  case VSI_NN_TYPE_INT16: ret += "16S:"; break;
  case VSI_NN_TYPE_FLOAT16: ret += "16F:"; break;
  case VSI_NN_TYPE_BFLOAT16: ret += "16B:"; break;
  case VSI_NN_TYPE_UINT32: ret += "32U:"; break;
  case VSI_NN_TYPE_INT32: ret += "32S:"; break;
  case VSI_NN_TYPE_FLOAT32: ret += "32F:"; break;
  case VSI_NN_TYPE_UINT64: ret += "64U:"; break;
  case VSI_NN_TYPE_INT64: ret += "64S:"; break;
  case VSI_NN_TYPE_FLOAT64: ret += "64F:"; break;
  default: ret += "TYPE_UNKNOWN";
  }

  // Dims:
  for (uint32_t i = 0; i < attr.dim_num; ++i)
    ret += std::to_string(attr.size[attr.dim_num - 1 - i]) + ((i<attr.dim_num-1) ? 'x' : ':');

  // Quantization:
  switch (attr.dtype.qnt_type)
  {
  case VSI_NN_QNT_TYPE_NONE: ret += "NONE"; break;
  case VSI_NN_QNT_TYPE_DFP: ret += "DFP:" + std::to_string(attr.dtype.fl); break;
  case VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC: // same value as VSI_NN_QNT_TYPE_AFFINE_SYMMETRIC:
    ret += "AA:" + std::to_string(attr.dtype.scale) + ':' + std::to_string(attr.dtype.zero_point);
    break;
  case  VSI_NN_QNT_TYPE_AFFINE_PERCHANNEL_SYMMETRIC: ret += "APS:unsupported"; break;
  default: ret += "QUANT_UNKNOWN";
  }

  return ret;
}

// ##############################################################################################################
vsi_nn_tensor_attr_t jevois::dnn::tensorattr(TfLiteTensor const * t)
{
  vsi_nn_tensor_attr_t attr; memset(&attr, 0, sizeof(attr));
  attr.dtype.fmt = VSI_NN_DIM_FMT_AUTO;
  attr.dtype.vx_type = jevois::dnn::tf2vsi(t->type);

  switch (t->quantization.type)
  {
  case kTfLiteNoQuantization:
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
    break;

  case kTfLiteAffineQuantization:
  {
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC;
    attr.dtype.scale = t->params.scale;
    attr.dtype.zero_point = t->params.zero_point;
  }
  break;
  
  default: LFATAL("unsupported quantization " << t->quantization.type);
  }
  
  TfLiteIntArray const & dims = *t->dims;
  attr.dim_num = dims.size;
  for (int i = 0; i < dims.size; ++i) attr.size[dims.size - 1 - i] = dims.data[i];

  // Set the fmt to NCHW or NHWC if possible:
  if (attr.dim_num == 4)
  {
    if (attr.size[0] > attr.size[2]) attr.dtype.fmt = VSI_NN_DIM_FMT_NCHW; // assume H>C
    else attr.dtype.fmt = VSI_NN_DIM_FMT_NHWC;
  }
  
  return attr;
}

// ##############################################################################################################
#ifdef JEVOIS_PRO
vsi_nn_tensor_attr_t jevois::dnn::tensorattr(hailo_vstream_info_t const & vi)
{
  vsi_nn_tensor_attr_t attr; memset(&attr, 0, sizeof(attr));

  attr.dtype.vx_type = hailo2vsi(vi.format.type);
  
  switch (vi.format.order)
  {
  case HAILO_FORMAT_ORDER_HAILO_NMS:
    attr.dtype.fmt = VSI_NN_DIM_FMT_AUTO;
    attr.dim_num = 2;
    attr.size[0] = vi.nms_shape.number_of_classes;
    attr.size[1] = vi.nms_shape.max_bboxes_per_class * 5; // Each box has: xmin, ymin, xmax, ymax, score
    break;
    
  case HAILO_FORMAT_ORDER_NHWC:
  case HAILO_FORMAT_ORDER_FCR:
  case HAILO_FORMAT_ORDER_F8CR:
    attr.dtype.fmt = VSI_NN_DIM_FMT_NHWC;
    attr.dim_num = 4;
    attr.size[0] = vi.shape.features;
    attr.size[1] = vi.shape.width;
    attr.size[2] = vi.shape.height;
    attr.size[3] = 1;
    break;
    
  case HAILO_FORMAT_ORDER_NHW:
    attr.dtype.fmt = VSI_NN_DIM_FMT_NHWC;
    attr.dim_num = 4;
    attr.size[0] = 1;
    attr.size[1] = vi.shape.width;
    attr.size[2] = vi.shape.height;
    attr.size[3] = 1;
    break;
    
  case HAILO_FORMAT_ORDER_NC:
    attr.dtype.fmt = VSI_NN_DIM_FMT_NHWC;
    attr.dim_num = 4;
    attr.size[0] = 1;
    attr.size[1] = 1;
    attr.size[2] = vi.shape.features;
    attr.size[3] = 1;
    break;
    
  case HAILO_FORMAT_ORDER_NCHW:
    attr.dtype.fmt = VSI_NN_DIM_FMT_NCHW;
    attr.dim_num = 4;
    attr.size[0] = vi.shape.features;
    attr.size[1] = vi.shape.width;
    attr.size[2] = vi.shape.height;
    attr.size[3] = 1;
    break;

  default: throw std::range_error("tensorattr: Unsupported Hailo order " +std::to_string(vi.format.order));
  }

  // Hailo only supports one quantization type:
  attr.dtype.qnt_type = VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC;
  attr.dtype.scale = vi.quant_info.qp_scale;
  attr.dtype.zero_point = int32_t(vi.quant_info.qp_zp);

  return attr;
}
#endif

// ##############################################################################################################
size_t jevois::dnn::softmax(float const * input, size_t const n, size_t const stride, float const fac, float * output,
                            bool maxonly)
{
  if (stride == 0) LFATAL("Cannot work with stride = 0");
  
  float sum = 0.0F;
  float largest = -FLT_MAX; size_t largest_idx = 0;
  size_t const ns = n * stride;

  for (size_t i = 0; i < ns; i += stride) if (input[i] > largest) { largest = input[i]; largest_idx = i; }

  if (fac == 1.0F)
    for (size_t i = 0; i < ns; i += stride)
    {
      float const e = expf(input[i] - largest);
      sum += e;
      output[i] = e;
    }
  else    
    for (size_t i = 0; i < ns; i += stride)
    {
      float const e = expf(input[i]/fac - largest/fac);
      sum += e;
      output[i] = e;
    }
  
  if (sum)
  {
    if (maxonly) output[largest_idx] /= sum;
    else for (size_t i = 0; i < ns; i += stride) output[i] /= sum;
  }
  
  return largest_idx;
}

// ##############################################################################################################
bool jevois::dnn::attrmatch(vsi_nn_tensor_attr_t const & attr, cv::Mat const & blob)
{
  // Check that blob and tensor are a complete match:
  if (blob.channels() != 1) return false;
  if (blob.depth() != jevois::dnn::vsi2cv(attr.dtype.vx_type)) return false;
  if (uint32_t(blob.size.dims()) != attr.dim_num) return false;

  for (size_t i = 0; i < attr.dim_num; ++i)
    if (int(attr.size[attr.dim_num - 1 - i]) != blob.size[i]) return false;

  return true;
}

// ##############################################################################################################
cv::Mat jevois::dnn::quantize(cv::Mat const & m, vsi_nn_tensor_attr_t const & attr)
{
  if (m.depth() != CV_32F) LFATAL("Tensor to quantize must be 32F");
  
  // Do a sloppy match for total size only since m may still be 2D RGB packed vs 4D attr...
  std::vector<int> adims = jevois::dnn::attrdims(attr);
  size_t tot = 1; for (int d : adims) tot *= d;
  
  if (tot != m.total() * m.channels())
    LFATAL("Mismatched tensor: " << jevois::dnn::shapestr(m) << " vs attr: " << jevois::dnn::shapestr(attr));

  unsigned int const tt = jevois::dnn::vsi2cv(attr.dtype.vx_type);

  switch (attr.dtype.qnt_type)
  {
  case VSI_NN_QNT_TYPE_NONE:
  {
    cv::Mat ret;
    m.convertTo(ret, tt);
    return ret;
  }

  case VSI_NN_QNT_TYPE_DFP:
  {
    switch (tt)
    {
    case CV_8S:
    {
      if (attr.dtype.fl > 7) LFATAL("Invalid DFP fl value " << attr.dtype.fl << ": must be in [0..7]");
      cv::Mat ret;
      m.convertTo(ret, tt, 1 << attr.dtype.fl, 0.0);
      return ret;
    }
    case CV_16S:
    {
      if (attr.dtype.fl > 15) LFATAL("Invalid DFP fl value " << attr.dtype.fl << ": must be in [0..15]");
      cv::Mat ret;
      m.convertTo(ret, tt, 1 << attr.dtype.fl, 0.0);
      return ret;
    }
    default:
      break;
    }
    break;
  }

  case VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC: // same value as VSI_NN_QNT_TYPE_AFFINE_SYMMETRIC:
  {
    switch (tt)
    {
    case CV_8U:
    {
      cv::Mat ret;
      if (attr.dtype.scale == 0.0) LFATAL("Quantization scale must not be zero in " << jevois::dnn::shapestr(attr));
      m.convertTo(ret, tt, 1.0 / attr.dtype.scale, attr.dtype.zero_point);
      return ret;
    }
    
    default:
      break;
    }
    break;
  }
  
  case  VSI_NN_QNT_TYPE_AFFINE_PERCHANNEL_SYMMETRIC:
    LFATAL("Affine per-channel symmetric not supported yet");
    
  default:
    break;
  }
  
  LFATAL("Quantization to " << jevois::dnn::shapestr(attr) << " not yet supported");
}

// ##############################################################################################################
cv::Mat jevois::dnn::dequantize(cv::Mat const & m, vsi_nn_tensor_attr_t const & attr)
{
  if (! jevois::dnn::attrmatch(attr, m))
    LFATAL("Mismatched tensor: " << jevois::dnn::shapestr(m) << " vs attr: " << jevois::dnn::shapestr(attr));

  switch (attr.dtype.qnt_type)
  {
  case VSI_NN_QNT_TYPE_NONE:
  {
    cv::Mat ret;
    m.convertTo(ret, CV_32F);
    return ret;
  }

  case VSI_NN_QNT_TYPE_DFP:
  {
    cv::Mat ret;
    m.convertTo(ret, CV_32F, 1.0 / (1 << attr.dtype.fl), 0.0);
    return ret;
  }
  
  case VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC: // same value as VSI_NN_QNT_TYPE_AFFINE_SYMMETRIC:
  {
    cv::Mat ret;
    m.convertTo(ret, CV_32F);
    if (attr.dtype.zero_point) ret -= attr.dtype.zero_point;
    if (attr.dtype.scale != 1.0F) ret *= attr.dtype.scale;
    return ret;
  }

  case  VSI_NN_QNT_TYPE_AFFINE_PERCHANNEL_SYMMETRIC:
    LFATAL("Affine per-channel symmetric not supported yet");

  default:
    LFATAL("Unknown quantization type " << int(attr.dtype.qnt_type));
  }
}

// ##############################################################################################################
size_t jevois::dnn::effectiveDims(cv::Mat const & m)
{
  cv::MatSize const & rs = m.size;
  size_t const ndims = rs.dims();
  size_t ret = ndims;
  for (size_t i = 0; i < ndims; ++i) if (rs[i] == 1) --ret; else break;
  return ret;
}


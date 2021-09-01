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
#include <regex>

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
    classname = std::regex_replace(classname, std::regex("\"\""), "\"");

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
  default:
    LFATAL("Unsupported type " << TfLiteTypeGetName(t));
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
  default:
    LFATAL("Unsupported type " << TfLiteTypeGetName(t));
  }
}

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
        throw std::range_error("parseTensorSpecs: In "+spec+", DFP quantization needs :fl param (" + specdef + ')');
      attr.dtype.fl = std::stoi(tok[n+1]);
    }
    
    else if (tok[n] == "AA")
    {
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC;
      if (tok.size() != n+3)
        throw std::range_error("parseTensorSpecs: In "+spec+", AA quantization needs :scale:zero params ("+specdef+')');
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
cv::Size jevois::dnn::attrsize(vsi_nn_tensor_attr_t const & attr)
{
  switch (attr.dtype.fmt)
  {
  case VSI_NN_DIM_FMT_NHWC:
    if (attr.dim_num < 3) throw std::range_error("attrsize: need at least 3D, got " + jevois::dnn::attrstr(attr));
    return cv::Size(attr.size[1], attr.size[2]);
    
  case VSI_NN_DIM_FMT_NCHW:
  default:
    if (attr.dim_num < 2) throw std::range_error("attrsize: need at least 2D, got " + jevois::dnn::attrstr(attr));
    return cv::Size(attr.size[0], attr.size[1]);
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
  default: std::range_error("attrstr: Unsupported tensor type " + std::to_string(attr.dtype.vx_type));
  }

  // Dims:
  for (uint32_t i = 0; i < attr.dim_num; ++i)
    ret += std::to_string(attr.size[attr.dim_num - 1 - i]) + ((i<attr.dim_num-1) ? 'x' : ':');

  // Quantization:
  switch (attr.dtype.qnt_type)
  {
  case VSI_NN_QNT_TYPE_NONE: ret += ":NONE"; break;
  case VSI_NN_QNT_TYPE_DFP: ret += ":DFP:" + std::to_string(attr.dtype.fl); break;
  case VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC: ret += ":AA:" + std::to_string(attr.dtype.scale) + ':' +
      std::to_string(attr.dtype.zero_point); break;
  case  VSI_NN_QNT_TYPE_AFFINE_PERCHANNEL_SYMMETRIC: ret += ":APS:unsupported"; break;
  default: std::range_error("attrstr: Unsupported tensor quantization " + std::to_string(attr.dtype.qnt_type));
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
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    //TfLiteAffineQuantization const * q = reinterpret_cast<TfLiteAffineQuantization const *>(t->quantization.params);
    //attr.dtype.scale = q->scale[0];//fixme
    //attr.dtype.zero_point = q->zero_point[0]; //fixme
    // FIXME q->quantized_dimension
  }
  break;
  
  default: LFATAL("unsupported quantization " << t->quantization.type);
  }
  
  TfLiteIntArray const & dims = *t->dims;
  attr.dim_num = dims.size;
  for (int i = 0; i < dims.size; ++i) attr.size[dims.size - 1 - i] = dims.data[i];

  return attr;
}

// ##############################################################################################################
void jevois::dnn::softmax(float const * input, size_t n, float fac, float * output)
{
  float sum = 0.0F;
  float largest = -FLT_MAX;
  for (size_t i = 0; i < n; ++i) if (input[i] > largest) largest = input[i];
  for (size_t i = 0; i < n; ++i)
  {
    float e = exp(input[i]/fac - largest/fac);
    sum += e;
    output[i] = e;
  }
  if (sum) for (size_t i = 0; i < n; ++i) output[i] /= sum;
}

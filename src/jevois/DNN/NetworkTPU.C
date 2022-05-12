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

#ifdef JEVOIS_PRO

#include <jevois/DNN/NetworkTPU.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <edgetpu_c.h>

#include <tensorflow/lite/builtin_op_data.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/kernels/internal/tensor_ctypes.h> // for GetTensorData()

// ####################################################################################################
int jevois::dnn::NetworkTPU::ErrorReporter::Report(char const * format, va_list args)
{
  static char buf[2048];
  int ret = vsnprintf(buf, 2048, format, args);
  itsErrors.push_back(buf);
  LERROR(buf);
  return ret;
}

// ####################################################################################################
jevois::dnn::NetworkTPU::~NetworkTPU()
{ waitBeforeDestroy(); }

// ####################################################################################################
void jevois::dnn::NetworkTPU::freeze(bool doit)
{
  dataroot::freeze(doit);
  model::freeze(doit);
  tpunum::freeze(doit);
  dequant::freeze(doit);
  intensors::freeze(doit);
  outtensors::freeze(doit);
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkTPU::inputShapes()
{
  if (ready() == false) LFATAL("Network is not ready");
  return jevois::dnn::parseTensorSpecs(intensors::get());
  /*
  std::vector<vsi_nn_tensor_attr_t> ret;
  auto const & input_indices = itsInterpreter->inputs();

  for (size_t i = 0; i < input_indices.size(); ++i)
  {
    TfLiteTensor const * itensor = itsInterpreter->tensor(input_indices[i]);
    if (itensor == nullptr) LFATAL("Network has Null input tensor " << i);
    ret.emplace_back(jevois::dnn::tensorattr(itensor));
  }
  return ret;
  */
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkTPU::outputShapes()
{
  if (ready() == false) LFATAL("Network is not ready");
  return jevois::dnn::parseTensorSpecs(outtensors::get());
  /*
  std::vector<vsi_nn_tensor_attr_t> ret;
  auto const & output_indices = itsInterpreter->outputs();

  for (size_t i = 0; i < output_indices.size(); ++i)
  {
    TfLiteTensor const * otensor = itsInterpreter->tensor(output_indices[i]);
    if (otensor == nullptr) LFATAL("Network has Null output tensor " << i);
    ret.emplace_back(jevois::dnn::tensorattr(otensor));
  }
  return ret;
  */
}

// ####################################################################################################
void jevois::dnn::NetworkTPU::load()
{
  // Need to nuke the network first if it exists or we could run out of RAM:
  itsInterpreter.reset();
  itsModel.reset();
  itsErrorReporter.itsErrors.clear();
  
  std::string const m = jevois::absolutePath(dataroot::get(), model::get());

  try
  {
    // Create and load the network:
    itsModel = tflite::FlatBufferModel::BuildFromFile(m.c_str(), &itsErrorReporter);
    if (!itsModel) LFATAL("Failed to load model from file " << m);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder(*itsModel, resolver)(&itsInterpreter);

    size_t num_devices;
    std::unique_ptr<edgetpu_device, decltype(&edgetpu_free_devices)>
      devices(edgetpu_list_devices(&num_devices), &edgetpu_free_devices);
    
    if (num_devices == 0) LFATAL("No connected TPU found");
    size_t const tn = tpunum::get();
    if (tn >= num_devices) LFATAL("Cannot use TPU " << tn << " because only " << num_devices << " TPUs detected.");
    
    auto const & device = devices.get()[tn];
    itsInterpreter->
      ModifyGraphWithDelegate(std::unique_ptr<TfLiteDelegate, decltype(&edgetpu_free_delegate)>
                              (edgetpu_create_delegate(device.type, device.path, nullptr, 0), &edgetpu_free_delegate));
    
    itsInterpreter->SetNumThreads(1);
    
    if (itsInterpreter->AllocateTensors() != kTfLiteOk) LFATAL("Failed to allocate tensors");
    
    for (size_t i = 0; i < itsInterpreter->inputs().size(); ++i)
      LINFO("Input tensor " << i << ": " << itsInterpreter->GetInputName(i));
    for (size_t i = 0; i < itsInterpreter->outputs().size(); ++i)
      LINFO("Output tensor " << i << ": " << itsInterpreter->GetOutputName(i));
    
    int t_size = itsInterpreter->tensors_size();
    for (int i = 0; i < t_size; ++i)
      if (itsInterpreter->tensor(i)->name)
        LINFO("Layer " << i << ": " << itsInterpreter->tensor(i)->name << ", "
              << jevois::dnn::shapestr(itsInterpreter->tensor(i)) << ", "
              << itsInterpreter->tensor(i)->bytes << " bytes, scale: "
              << itsInterpreter->tensor(i)->params.scale << ", zero: "
              << itsInterpreter->tensor(i)->params.zero_point);
    
    //if (threads::get()) itsInterpreter->SetNumThreads(threads::get());
    for (size_t i = 0; i < itsInterpreter->inputs().size(); ++i)
      LINFO("input " << i << " is layer " << itsInterpreter->inputs()[i]);
    for (size_t i = 0; i < itsInterpreter->outputs().size(); ++i)
      LINFO("output " << i << " is layer " << itsInterpreter->outputs()[i]);
  }
  catch (std::exception const & e)
  {
    std::string err = "\n";
    for (std::string const & s : itsErrorReporter.itsErrors) err += "ERR " + s + "\n";
    err += e.what();
    throw std::runtime_error(err);
  }
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkTPU::doprocess(std::vector<cv::Mat> const & blobs,
                                                        std::vector<std::string> & info)
{
  if ( ! itsInterpreter) LFATAL("Internal inconsistency");

  if (blobs.size() != itsInterpreter->inputs().size())
    LFATAL("Received " << blobs.size() << " input tensors, but network wants " << itsInterpreter->inputs().size());
  
  auto const & input_indices = itsInterpreter->inputs();
  for (size_t b = 0; b < blobs.size(); ++b)
  {
    cv::Mat const & cvin = blobs[b];
    auto * itensor = itsInterpreter->tensor(input_indices[b]);
    if (itensor == nullptr) LFATAL("Network has Null input tensor " << b);

    // Make sure input dims are a match:
    TfLiteIntArray const & tfindims = *itensor->dims;
    cv::MatSize const & cvindims = cvin.size;
    for (int i = 0; i < tfindims.size; ++i)
      if (tfindims.data[i] != cvindims[i])
        LFATAL("Input " << b << " mismatch: blob is " << jevois::dnn::shapestr(cvin) <<
               " but network wants " << jevois::dnn::shapestr(itensor));

    // Make sure total sizes in bytes are a match too:
    size_t const cvsiz = cvin.total() * cvin.elemSize();
    size_t const tfsiz = itensor->bytes;
    if (cvsiz != tfsiz) LFATAL("Input " << b << " size mismatch: blob has " << cvsiz <<
                               " but network wants " << tfsiz << " bytes. Maybe type is wrong in intensors?");

    // Copy input blob to input tensor:
    uint8_t * input = tflite::GetTensorData<uint8_t>(itensor);
    if (input == nullptr) LFATAL("Input tensor " << b << " is null in network");
    std::memcpy(input, cvin.data, cvsiz);
    info.emplace_back("- Input tensors ok");
  }

  // Run the network:
  if (itsInterpreter->Invoke() != kTfLiteOk) LFATAL("Failed to invoke interpreter");
  info.emplace_back("- Network forward pass ok");

  // Collect/convert the outputs:
  auto const & output_indices = itsInterpreter->outputs();
  std::vector<cv::Mat> outs;

  for (size_t o = 0; o < output_indices.size(); ++o)
  {
    auto const * otensor = itsInterpreter->tensor(output_indices[o]);
    if (otensor == nullptr) LFATAL("Network produced Null output tensor " << o);

    // Allocate an OpenCV output array of dims that match our output tensor:
    TfLiteIntArray const & tfdims = *otensor->dims;
    std::vector<int> cvdims; size_t sz = 1;
    for (int i = 0; i < tfdims.size; ++i) { cvdims.emplace_back(tfdims.data[i]); sz *= tfdims.data[i]; }

    // Convert/copy output tensor data to OpenCV arrays:
    TfLiteType const ot = otensor->type;
    std::string const otname = TfLiteTypeGetName(ot);
    bool notdone = true;
    
    if (dequant::get())
    {
      switch (ot)
      {
      case kTfLiteUInt8:
      {
        // Dequantize UINT8 to FLOAT32:
        uint8_t const * output = tflite::GetTensorData<uint8_t>(otensor);
        if (output == nullptr) LFATAL("Network produced Null output tensor data " << o);
        cv::Mat cvout(cvdims, CV_32F); float * cvoutdata = (float *)cvout.data;

        for (size_t i = 0; i < sz; ++i)
          *cvoutdata++ = (output[i] - otensor->params.zero_point) * otensor->params.scale;

        info.emplace_back("- Converted " + otname + " output tensor " + std::to_string(o) + " to FLOAT32");
        outs.emplace_back(cvout);
        notdone = false;
      }
      break;
      
      default:
        // For now, we only know how to dequantize uint8...
        break;
      }
    }
    
    if (notdone)
    {
      // We just want to copy the data untouched, except that OpenCV does not support as many pixel types as tensorflow:
      switch (ot)
      {
      case kTfLiteInt64: // used by DeepLabV3. Just convert to int32:
      {
        cv::Mat cvout(cvdims, CV_32S);
        int * cvoutdata = (int *)cvout.data;
        int64_t const * output = tflite::GetTensorData<int64_t>(otensor);
        if (output == nullptr) LFATAL("Network produced Null output tensor data " << o);
        for (size_t i = 0; i < sz; ++i) *cvoutdata++ = int(*output++);
        info.emplace_back("- Converted " + otname + " output tensor " + std::to_string(o) + " to INT32");
        outs.emplace_back(cvout);
      }
      break;

      case kTfLiteFloat32:
      case kTfLiteInt32:
      case kTfLiteUInt8:
      case kTfLiteInt16:
      case kTfLiteInt8:
      case kTfLiteFloat16:
      case kTfLiteFloat64:
      {
        // Simple copy with no conversion:
        unsigned int cvtype = jevois::dnn::tf2cv(ot);
        cv::Mat cvout(cvdims, cvtype);
        uint8_t const * output = tflite::GetTensorData<uint8_t>(otensor);
        if (output == nullptr) LFATAL("Network produced Null output tensor data " << o);
        std::memcpy(cvout.data, output, sz * jevois::cvBytesPerPix(cvtype));
        info.emplace_back("- Copied " + otname + " output tensor " + std::to_string(o));
        outs.emplace_back(cvout);
      }
      break;
      
      default:
        LFATAL("Output tensor " << otensor->name << " has unsupported type: " << otname);
      }
    }
  }

  // Report the TPU temperature:
  size_t tn = tpunum::get();
  std::string fn = jevois::sformat("/sys/class/apex/apex_%zu/temp", tpunum::get());
  try
  {
    int temp = std::stoi(jevois::getFileString(fn.c_str()));
    info.emplace_back(jevois::sformat("- TPU%zu temp %dC", tn, temp / 1000));
  }
  catch (...) { } // silently ignore any errors
  
  return outs;
}

#endif // JEVOIS_PRO

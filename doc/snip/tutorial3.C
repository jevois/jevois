#include <jevois/Core/Module.H>
#include <jevois/Image/RawImageOps.H>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// Parameters for our module:
static jevois::ParameterCategory const ParamCateg("Edge Detection Options");

JEVOIS_DECLARE_PARAMETER(thresh1, double, "First threshold for hysteresis", 50.0, ParamCateg);
JEVOIS_DECLARE_PARAMETER(thresh2, double, "Second threshold for hysteresis", 150.0, ParamCateg);
JEVOIS_DECLARE_PARAMETER(aperture, int, "Aperture size for the Sobel operator", 3, jevois::Range<int>(3, 53), ParamCateg);
JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(l2grad, bool, "Use more accurate L2 gradient norm if true, L1 if false", false, ParamCateg);

// Simple module to detect edges using the Canny algorithm from OpenCV
class EdgeDetection : public jevois::Module,
                      public jevois::Parameter<thresh1, thresh2, aperture, l2grad>
{
  public:
    // Default base class constructor ok
    using jevois::Module::Module;

    // Virtual destructor for safe inheritance
    virtual ~EdgeDetection() { }

    // Processing function
    virtual void process(jevois::InputFrame && inframe, jevois::OutputFrame && outframe) override
    {
      // Wait for next available camera image:
      jevois::RawImage inimg = inframe.get();

      // Convert to OpenCV grayscale:
      cv::Mat grayimg = jevois::rawimage::convertToCvGray(inimg);
  
      // Let camera know we are done processing the input image:
      inframe.done();

      // Wait for an image from our gadget driver into which we will put our results. Require that it must have same
      // image size as the input image, and greyscale pixels:
      jevois::RawImage outimg = outframe.get();
      outimg.require("output", inimg.width, inimg.height, V4L2_PIX_FMT_GREY);

      // Compute Canny edges directly into the output image:
      cv::Mat edges = jevois::rawimage::cvImage(outimg); // Pixel data of "edges" shared with "outimg", no copy
      cv::Canny(grayimg, edges, thresh1::get(), thresh2::get(), aperture::get(), l2grad::get());
      
      // Send the output image with our processing results to the host over USB:
      outframe.send();
    }

    // Callback function for parameter l2grad
    void onParamChange(l2grad const & param, bool const & newval) override
    {
      LINFO("you changed l2grad to be " + std::to_string(newval));
    }
};

// Allow the module to be loaded as a shared object (.so) file:
JEVOIS_REGISTER_MODULE(EdgeDetection);

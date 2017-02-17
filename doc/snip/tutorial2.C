#include <jevois/Core/Module.H>
#include <jevois/Image/RawImageOps.H>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// Simple module to convert between any supported camera grab formats and USB output formats
class Convert : public jevois::Module
{
  public:
    // Default base class constructor ok
    using jevois::Module::Module;

    // Virtual destructor for safe inheritance
    virtual ~Convert() { }

    // Processing function
    virtual void process(jevois::InputFrame && inframe, jevois::OutputFrame && outframe) override
    {
      // Wait for next available camera image:
      jevois::RawImage inimg = inframe.get();

      // Convert it to BGR24:
      cv::Mat imgbgr = jevois::rawimage::convertToCvBGR(inimg);
  
      // Let camera know we are done processing the input image:
      inframe.done();
      
      // Wait for an image from our gadget driver into which we will put our results:
      jevois::RawImage outimg = outframe.get();

      // Require that output has same dims as input, allow any output format:
      outimg.require("output", inimg.width, inimg.height, outimg.fmt);

      // Convert from BGR to desired output format:
      jevois::rawimage::convertCvBGRtoRawImage(imgbgr, outimg);
      
      // Send the output image with our processing results to the host over USB:
      outframe.send();
    }
};

// Allow the module to be loaded as a shared object (.so) file:
JEVOIS_REGISTER_MODULE(Convert);

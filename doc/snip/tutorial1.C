#include <jevois/Core/Module.H>

// Simple module that just passes the captured camera frames through to USB host
class TutorialPassThrough : public jevois::Module
{
  public:
    // Default base class constructor ok
    using jevois::Module::Module;

    // Virtual destructor for safe inheritance
    virtual ~TutorialPassThrough() { }

    // Processing function
    virtual void process(jevois::InputFrame && inframe, jevois::OutputFrame && outframe) override
    {
      // Wait for next available camera image:
      jevois::RawImage const inimg = inframe.get(true);
      
      // Wait for an image from our gadget driver into which we will put our results:
      jevois::RawImage outimg = outframe.get();

      // Enforce that the input and output formats and image sizes match:
      outimg.require("output", inimg.width, inimg.height, inimg.fmt);
      
      // Just copy the pixel data over:
      memcpy(outimg.pixelsw<void>(), inimg.pixels<void>(), outimg.bytesize());

      // Let camera know we are done processing the input image:
      inframe.done(); // NOTE: optional here, inframe destructor would call it anyway

      // Send the output image with our processing results to the host over USB:
      outframe.send(); // NOTE: optional here, outframe destructor would call it anyway
    }
};

// Allow the module to be loaded as a shared object (.so) file:
JEVOIS_REGISTER_MODULE(TutorialPassThrough);

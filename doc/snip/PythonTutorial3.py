import libjevois as jevois

## Simple test of programming JeVois modules in Python
#
# This module by default simply draws a cricle and a text message onto the grabbed video frames.
#
# Feel free to edit it and try something else. Note that this module does not import OpenCV, see the PythonOpenCV for a
# minimal JeVois module written in Python that uses OpenCV.
#
# @author Laurent Itti
#
# @displayname Python Tutorial 3
# @videomapping YUYV 640 480 15.0 YUYV 640 480 15.0 JeVois PythonTutorial3
# @email itti\@usc.edu
# @address University of Southern California, HNB-07A, 3641 Watt Way, Los Angeles, CA 90089-2520, USA
# @copyright Copyright (C) 2017 by Laurent Itti, iLab and the University of Southern California
# @mainurl http://jevois.org
# @supporturl http://jevois.org/doc
# @otherurl http://iLab.usc.edu
# @license GPL v3
# @distribution Unrestricted
# @restrictions None
# @ingroup modules
class PythonTTutorial3:
    # ###################################################################################################
    ## Constructor
    def __init__(self):
        jevois.LINFO("PythonTest Constructor")
        jevois.LINFO(dir(jevois))
        self.frame = 0 # a simple frame counter used to demonstrate sendSerial()

    # ###################################################################################################
    ## Process function with no USB output
    def process(self, inframe):
        jevois.LFATAL("process no usb not implemented")

    # ###################################################################################################
    ## Process function with USB output
    def process(self, inframe, outframe):
        jevois.LINFO("process with usb")

        # Get the next camera image (may block until it is captured):
        inimg = inframe.get()
        jevois.LINFO("Input image is {} {}x{}".format(jevois.fccstr(inimg.fmt), inimg.width, inimg.height))

        # Get the next available USB output image:
        outimg = outframe.get()
        jevois.LINFO("Output image is {} {}x{}".format(jevois.fccstr(outimg.fmt), outimg.width, outimg.height))

        # Example of getting pixel data from the input and copying to the output:
        jevois.paste(inimg, outimg, 0, 0)

        # We are done with the input image:
        inframe.done()

        # Example of in-place processing:
        jevois.hFlipYUYV(outimg)

        # Example of simple drawings:
        jevois.drawCircle(outimg, int(outimg.width/2), int(outimg.height/2), int(outimg.height/2.2), 2, 0x80ff)
        jevois.writeText(outimg, "Hi from Python!", 20, 20, 0x80ff, jevois.Font.Font10x20)
        
        # We are done with the output, ready to send it to host over USB:
        outframe.send()

        # Send a string over serial (e.g., to an Arduino). Remember to tell the JeVois Engine to display those messages,
        # as they are turned off by default. For example: 'setpar serout All' in the JeVois console:
        jevois.sendSerial("DONE frame {}".format(self.frame));
        self.frame += 1

    # ###################################################################################################
    ## Parse a serial command forwarded to us by the JeVois Engine, return a string
    def parseSerial(self, str):
        jevois.LINFO("parseserial received command [{}]".format(str))
        if str == "hello":
            return self.hello()
        return "ERR: Unsupported command"
    
    # ###################################################################################################
    ## Return a string that describes the custom commands we support, for the JeVois help message
    def supportedCommands(self):
        # use \n seperator if your module supports several commands
        return "hello - print hello using python"

    # ###################################################################################################
    ## Internal method that gets invoked as a custom command
    def hello(self):
        return "Hello from python!"
        

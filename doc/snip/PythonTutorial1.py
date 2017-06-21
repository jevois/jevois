import libjevois as jevois
import cv2
import numpy as np

## Simple example of image processing using OpenCV in Python on JeVois
#
# This module by default simply converts the input image to a grayscale OpenCV image, and then applies the Canny
# edge detection algorithm. Try to edit it to do something else (note that the videomapping associated with this
# module has grayscale image outputs, so that is what you should output).
#
# @author Laurent Itti
# 
# @displayname Python Tutorial 1
# @videomapping GRAY 640 480 20.0 YUYV 640 480 20.0 JeVois PythonOpenCV
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
class PythonTutorial1:
    # ###################################################################################################
    ## Process function with USB output
    def process(self, inframe, outframe):
        # Get the next camera image (may block until it is captured) and convert it to OpenCV GRAY:
        inimggray = inframe.getCvGRAY()
        
        # Detect edges using the Canny algorithm from OpenCV:
        edges = cv2.Canny(inimggray, 100, 200, apertureSize = 3)
        
        # Convert our GRAY output image to video output format and send to host over USB:
        outframe.sendCvGRAY(edges)

import pyjevois
if pyjevois.pro: import libjevoispro as jevois
else: import libjevois as jevois
import cv2
import numpy as np

## __SYNOPSIS__
#
# This module is here for you to experiment with Python OpenCV on JeVois and JeVois-Pro.
#
# By default, we get the next video frame from the camera as an OpenCV BGR (color) image named 'inimg'.
# We then apply some image processing to it to create an overlay in Pro/GUI mode, an output BGR image named
# 'outimg' in Legacy mode, or no image in Headless mode.
#
# - In Legacy mode (JeVois-A33 or JeVois-Pro acts as a webcam connected to a host): process() is called on every
#   frame. A video frame from the camera sensor is given in 'inframe' and the process() function create an output frame
#   that is sent over USB to the host computer (JeVois-A33) or displayed (JeVois-Pro).
#   
# - In Pro/GUI mode (JeVois-Pro is connected to an HDMI display): processGUI() is called on every frame. A video frame
#   from the camera is given, as well as a GUI helper that can be used to create overlay drawings.
#
# - In Headless mode (JeVois-A33 or JeVois-Pro only produces text messages over serial port, no video output):
#   processNoUSB() is called on every frame. A video frame from the camera is given, and the module sends messages over
#   serial to report what it sees.
#
# Which mode is activated depends on which VideoMapping was selected by the user. The VideoMapping specifies camera
# format and framerate, and what kind of mode and output format to use.
#
# See http://jevois.org/tutorials for tutorials on getting started with programming JeVois in Python without having
# to install any development software on your host computer.
#
# @author __AUTHOR__
# 
# @videomapping __VIDEOMAPPING__
# @email __EMAIL__
# @address fixme
# @copyright Copyright (C) 2021 by __AUTHOR__
# @mainurl __WEBSITE__
# @supporturl 
# @otherurl 
# @license __LICENSE__
# @distribution Unrestricted
# @restrictions None
# @ingroup modules
class __MODULE__:
    # ###################################################################################################
    ## Constructor
    def __init__(self):
        # Instantiate a JeVois Timer to measure our processing framerate:
        self.timer = jevois.Timer("timer", 100, jevois.LOG_INFO)

        # Create an ArUco marker detector:
        self.dict = cv2.aruco.Dictionary_get(cv2.aruco.DICT_4X4_50)
        self.params = cv2.aruco.DetectorParameters_create()
 
    # ###################################################################################################
    ## Process function with USB output (Legacy mode):
    def process(self, inframe, outframe):
        # Get the next camera image for processing (may block until it is captured) and here convert it to OpenCV BGR by
        # default. If you need a grayscale image instead, just use getCvGRAYp() instead of getCvBGRp(). Also supported
        # are getCvRGBp() and getCvRGBAp():
        inimg = inframe.getCvBGRp()
        
        # Start measuring image processing time (NOTE: does not account for input conversion time):
        self.timer.start()

        # Detect edges using the Laplacian algorithm from OpenCV:
        #
        # Replace the line below by your own code! See for example
        # - http://docs.opencv.org/trunk/d4/d13/tutorial_py_filtering.html
        # - http://docs.opencv.org/trunk/d9/d61/tutorial_py_morphological_ops.html
        # - http://docs.opencv.org/trunk/d5/d0f/tutorial_py_gradients.html
        # - http://docs.opencv.org/trunk/d7/d4d/tutorial_py_thresholding.html
        #
        # and so on. When they do "img = cv2.imread('name.jpg', 0)" in these tutorials, the last 0 means they want a
        # gray image, so you should use getCvGRAY() above in these cases. When they do not specify a final 0 in imread()
        # then usually they assume color and you should use getCvBGRp() here.
        #
        # The simplest you could try is:
        #    outimg = inimg
        # which will make a simple copy of the input image to output.
        outimg = cv2.Laplacian(inimg, -1, ksize=5, scale=0.25, delta=127)

        # Also detect and draw ArUco markers:
        grayimg = cv2.cvtColor(inimg, cv2.COLOR_BGR2GRAY)
        corners, ids, rej = cv2.aruco.detectMarkers(grayimg, self.dict, parameters = self.params)
        outimg = cv2.aruco.drawDetectedMarkers(outimg, corners, ids)
        
        # Write a title:
        cv2.putText(outimg, "JeVois Python Sandbox", (3, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255,255,255))
        
        # Write frames/s info from our timer into the edge map (NOTE: does not account for output conversion time):
        fps = self.timer.stop()
        outheight = outimg.shape[0]
        outwidth = outimg.shape[1]
        cv2.putText(outimg, fps, (3, outheight - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255,255,255))

        # Convert our OpenCv output image to video output format and send to host over USB:
        outframe.sendCv(outimg)

    # ###################################################################################################
    ## Process function with GUI output (JeVois-Pro mode):
    def processGUI(self, inframe, helper):
        # Start a new display frame, gets its size and also whether mouse/keyboard are idle:
        idle, winw, winh = helper.startFrame()

        # Draw full-resolution color input frame from camera. It will be automatically centered and scaled to fill the
        # display without stretching it. The position and size are returned, but often it is not needed as JeVois
        # drawing functions will also automatically scale and center. So, when drawing overlays, just use image
        # coordinates and JeVois will convert them to display coordinates automatically:
        x, y, iw, ih = helper.drawInputFrame("c", inframe, False, False)
        
        # Get the next camera image for processing (may block until it is captured), as greyscale:
        inimg = inframe.getCvGRAYp()

        # Start measuring image processing time (NOTE: does not account for input conversion time):
        self.timer.start()

        # Detect edges using the Canny algorithm from OpenCV:
        #
        # Replace the line below by your own code! See for example
        # - https://docs.opencv.org/master/da/d22/tutorial_py_canny.html
        # - http://docs.opencv.org/trunk/d4/d13/tutorial_py_filtering.html
        # - http://docs.opencv.org/trunk/d9/d61/tutorial_py_morphological_ops.html
        # - http://docs.opencv.org/trunk/d5/d0f/tutorial_py_gradients.html
        # - http://docs.opencv.org/trunk/d7/d4d/tutorial_py_thresholding.html
        #
        # and so on. When they do "img = cv2.imread('name.jpg', 0)" in these tutorials, the last 0 means they want a
        # gray image, so you should use getCvGRAYp() above in these cases. When they do not specify a final 0 in
        # imread() then usually they assume BGR color and you should use getCvBGRp() here.
        edges = cv2.Canny(inimg, 100, 200)

        # Edges is a greyscale image. To display it as an overlay, we convert it to RGBA, with zero alpha (transparent)
        # in background and full alpha on edges. We just duplicate our edge map 4 times for A, B, G, R:
        mask = cv2.merge([edges, edges, edges, edges])

        # Draw the edges as an overlay on top of the full-resolution camera input frame. It will automatically be
        # re-scaled and centered to match the last-drawn full-resolution frame:
        # Flags here are: rgb = True, noalias = False, isoverlay=True
        helper.drawImage("edges", mask, True, False, True)
        
        # Examples of some GUI overlay drawings. Draw color format in hex: 0xAABBGGRR where AA is the alpha (typically
        # keep at ff). The last 'True' parameter is to draw a semi-transparent filled shape.
        helper.drawCircle(50, 50, 20, 0xff80ffff, True)
        helper.drawRect(100, 100, 300, 200, 0xffff80ff, True)

        # Also detect and draw ArUco markers:
        corners, ids, rej = cv2.aruco.detectMarkers(inimg, self.dict, parameters = self.params)
        if len(corners) > 0:
            for (marker, id) in zip(corners, ids):
                helper.drawPoly(marker, 0xffff0000, True)
                helper.drawText(float(marker[0][0][0]), float(marker[0][0][1]), "id={}".format(id), 0xffff0000)
            
        # Write frames/s info from our timer:
        fps = self.timer.stop()
        helper.iinfo(inframe, fps, winw, winh);

        # End of frame:
        helper.endFrame()
        
    # ###################################################################################################
    ## Process function with no USB output (Headless mode):
    def processNoUSB(self, inframe):
        # Get the next camera image at the processing resolution (may block until it is captured) and here convert it to
        # OpenCV GRAY by default. Also supported are getCvRGBp(), getCvBGRp(), and getCvRGBAp():
        inimg = inframe.getCvGRAYp()

        # Detect ArUco markers:
        corners, ids, rej = cv2.aruco.detectMarkers(inimg, self.dict, parameters = self.params)

        # Nothing to display in headless mode. Instead, just send some data to serial port:
        if len(corners) > 0:
            for (marker, id) in zip(corners, ids):
                jevois.sendSerial("Detected ArUco ID={}".format(id))

        

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
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

#include <jevois/Core/MovieOutput.H>
#include <jevois/Debug/Log.H>

#include <opencv2/imgproc/imgproc.hpp>

#include <linux/videodev2.h> // for v4l2 pixel types
#include <cstdlib> // for std::system()
#include <cstdio> // for snprintf()
#include <fstream>

static char const PATHPREFIX[] = "/jevois/data/movieout/";

// ####################################################################################################
jevois::MovieOutput::MovieOutput(std::string const & fn) :
    itsBuf(1000), itsSaving(false), itsFileNum(0), itsRunning(true), itsFilebase(fn)
{
  itsRunFut = std::async(std::launch::async, &jevois::MovieOutput::run, this);
}

// ####################################################################################################
jevois::MovieOutput::~MovieOutput()
{
  // Signal end of run:
  itsRunning.store(false);
      
  // Push an empty frame into our buffer to signal the end of video to our thread:
  itsBuf.push(cv::Mat());

  // Wait for the thread to complete:
  LINFO("Waiting for writer thread to complete, " << itsBuf.filled_size() << " frames to go...");
  try { itsRunFut.get(); } catch (...) { jevois::warnAndIgnoreException(); }
  LINFO("Writer thread completed. Syncing disk...");
  if (std::system("/bin/sync")) LERROR("Error syncing disk -- IGNORED");
  LINFO("Video " << itsFilename << " saved.");
}

// ##############################################################################################################
void jevois::MovieOutput::setFormat(VideoMapping const & m)
{
  // Store the mapping so we can check frame size and format when giving out our buffer:
  itsMapping = m;
}

// ##############################################################################################################
void jevois::MovieOutput::get(RawImage & img)
{
  if (itsSaving.load())
  {
    // Reset our VideoBuf using the current format:
    itsBuffer.reset(new jevois::VideoBuf(-1, itsMapping.osize(), 0));

    img.width = itsMapping.ow;
    img.height = itsMapping.oh;
    img.fmt = itsMapping.ofmt;
    img.fps = itsMapping.ofps;
    img.buf = itsBuffer;
    img.bufindex = 0;
  }
  else LFATAL("Cannot get() while not streaming");
}

// ##############################################################################################################
void jevois::MovieOutput::send(RawImage const & img)
{
  if (itsSaving.load())
  {
    // Our thread will do the actual encoding:
    if (itsBuf.filled_size() > 1000) LERROR("Image queue too large, video writer cannot keep up - DROPPING FRAME");
    else itsBuf.push(jevois::rawimage::convertToCvBGR(img));

    // Nuke our buf:
    itsBuffer.reset();
  }
  else LFATAL("Aborting send() while not streaming");
}

// ##############################################################################################################
void jevois::MovieOutput::streamOn()
{
  itsSaving.store(true);
}

// ##############################################################################################################
void jevois::MovieOutput::abortStream()
{
  itsSaving.store(false);
}

// ##############################################################################################################
void jevois::MovieOutput::streamOff()
{
  itsSaving.store(false);

  // Push an empty frame into our buffer to signal the end of video to our thread:
  itsBuf.push(cv::Mat());

  // Wait for the thread to empty our image buffer:
  while (itsBuf.filled_size())
  {
    LINFO("Waiting for writer thread to complete, " << itsBuf.filled_size() << " frames to go...");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  LINFO("Writer thread completed. Syncing disk...");
  if (std::system("/bin/sync")) LERROR("Error syncing disk -- IGNORED");
  LINFO("Video " << itsFilename << " saved.");
}

// ##############################################################################################################
void jevois::MovieOutput::run() // Runs in a thread
{
  while (itsRunning.load())
  {
    // Create a VideoWriter here, since it has no close() function, this will ensure it gets destroyed and closes
    // the movie once we stop the recording:
    cv::VideoWriter writer;
    int frame = 0;
      
    while (true)
    {
      // Get next frame from the buffer:
      cv::Mat im = itsBuf.pop();

      // An empty image will be pushed when we are ready to close the video file:
      if (im.empty()) break;
        
      // Start the encoder if it is not yet running:
      if (writer.isOpened() == false)
      {
        std::string const fcc = "MJPG";
        //std::string const fcc = "MP4V";
        int const cvfcc = cv::VideoWriter::fourcc(fcc[0], fcc[1], fcc[2], fcc[3]);
          
        // Add path prefix if given filename is relative:
        std::string fn = itsFilebase;
        if (fn.empty()) LFATAL("Cannot save to an empty filename");
        if (fn[0] != '/') fn = PATHPREFIX + fn;

        // Create directory just in case it does not exist:
        std::string const cmd = "/bin/mkdir -p " + fn.substr(0, fn.rfind('/'));
        if (std::system(cmd.c_str())) LERROR("Error running [" << cmd << "] -- IGNORED");

        // Fill in the file number; be nice and do not overwrite existing files:
        while (true)
        {
          char tmp[2048];
          std::snprintf(tmp, 2047, fn.c_str(), itsFileNum);
          std::ifstream ifs(tmp);
          if (ifs.is_open() == false) { itsFilename = tmp; break; }
          ++itsFileNum;
        }
            
        // Open the writer:
        if (writer.open(itsFilename, cvfcc, itsMapping.ofps, im.size(), true) == false)
          LFATAL("Failed to open video encoder for file [" << itsFilename << ']');
      }
      
      // Write the frame:
      writer << im;
      
      // Report what is going on once in a while:
      if ((++frame % 100) == 0) LINFO("Written " << frame << " video frames");
    }
    
    // Our writer runs out of scope and closes the file here.
    ++itsFileNum;
  }
}

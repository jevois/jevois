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

#include <jevois/Core/Module.H>
#include <jevois/Core/VideoInput.H>
#include <jevois/Core/VideoOutput.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/UserInterface.H>

// ####################################################################################################
jevois::InputFrame::InputFrame(std::shared_ptr<jevois::VideoInput> const & cam, bool turbo) :
    itsCamera(cam), itsDidGet(false), itsDidDone(false), itsTurbo(turbo)
{ }

// ####################################################################################################
jevois::InputFrame::~InputFrame()
{
  // If itsCamera is invalidated, we have been moved to another object, so do not do anything here:
  if (itsCamera.get() == nullptr) return;
  
  // If we did not yet get(), just end now, camera will drop this frame:
  if (itsDidGet == false) return;
  
  // If we did get() but not done(), signal done now:
  if (itsDidDone == false) try { itsCamera->done(itsImage); } catch (...) { }
}

// ####################################################################################################
jevois::RawImage const & jevois::InputFrame::get(bool casync) const
{
  itsCamera->get(itsImage);
  itsDidGet = true;
  if (casync && itsTurbo) itsImage.buf->sync();
  return itsImage;
}

// ####################################################################################################
void jevois::InputFrame::done() const
{
  itsCamera->done(itsImage);
  itsDidDone = true;
}

// ####################################################################################################
// ####################################################################################################
jevois::OutputFrame::OutputFrame(std::shared_ptr<jevois::VideoOutput> const & gad) :
    itsGadget(gad), itsDidGet(false), itsDidSend(false)
{ }

// ####################################################################################################
jevois::OutputFrame::~OutputFrame()
{
  // If itsGadget is invalidated, we have been moved to another object, so do not do anything here:
  if (itsGadget.get() == nullptr) return;

  // If we did not get(), just end now:
  if (itsDidGet == false) return;

  // If we did get() but not send(), send now (the image will likely contain garbage):
  if (itsDidSend == false) try { itsGadget->send(itsImage); } catch (...) { }
}

// ####################################################################################################
jevois::RawImage const & jevois::OutputFrame::get() const
{
  itsGadget->get(itsImage);
  itsDidGet = true;
  return itsImage;
}

// ####################################################################################################
void jevois::OutputFrame::send() const
{
  itsGadget->send(itsImage);
  itsDidSend = true;
}

// ####################################################################################################
// ####################################################################################################
jevois::Module::Module(std::string const & instance) :
    jevois::Component(instance)
{ }

// ####################################################################################################
jevois::Module::~Module()
{ }

// ####################################################################################################
void jevois::Module::process(InputFrame && JEVOIS_UNUSED_PARAM(inframe), OutputFrame && JEVOIS_UNUSED_PARAM(outframe))
{ LFATAL("Not implemented in this module"); }

// ####################################################################################################
void jevois::Module::process(InputFrame && JEVOIS_UNUSED_PARAM(inframe))
{ LFATAL("Not implemented in this module"); }

// ####################################################################################################
void jevois::Module::sendSerial(std::string const & str)
{
  jevois::Engine * e = dynamic_cast<jevois::Engine *>(itsParent);
  if (e == nullptr) LFATAL("My parent is not Engine -- CANNOT SEND SERIAL");

  e->sendSerial(str);
}

// ####################################################################################################
void jevois::Module::parseSerial(std::string const & JEVOIS_UNUSED_PARAM(str),
                                 std::shared_ptr<jevois::UserInterface> JEVOIS_UNUSED_PARAM(s))
{ throw std::runtime_error("Unsupported command"); }

// ####################################################################################################
void jevois::Module::supportedCommands(std::ostream & os)
{ os << "None" << std::endl; }


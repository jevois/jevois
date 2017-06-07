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

#include <jevois/Core/PythonModule.H>
#include <jevois/Core/UserInterface.H>

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################

jevois::InputFramePython::InputFramePython(InputFrame * src) : itsInputFrame(src)
{ }

jevois::RawImage const & jevois::InputFramePython::get1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->get(casync);
}

jevois::RawImage const & jevois::InputFramePython::get() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->get(false);
}

void jevois::InputFramePython::done() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  itsInputFrame->done();
}
 
// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::OutputFramePython::OutputFramePython(OutputFrame * src) : itsOutputFrame(src)
{ }

jevois::RawImage const & jevois::OutputFramePython::get() const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  return itsOutputFrame->get();
}

void jevois::OutputFramePython::send() const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->send();
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::PythonModule::PythonModule(jevois::VideoMapping const & m) :
    jevois::Module(m.modulename)
{
  if (m.ispython == false) LFATAL("Passed video mapping is not for a python module");

  try
  {
    // Get the python interpreter going:
    itsMainModule = boost::python::import("__main__");
    itsMainNamespace = itsMainModule.attr("__dict__");

    // Import the module. Note that we import the whole directory:
    std::string const pypath = m.sopath();
    std::string const pydir = pypath.substr(0, pypath.rfind('/'));
    std::string const execstr =
      "import sys\n"
      "sys.path.append(\"" JEVOIS_ROOT_PATH "/lib\")\n" // To find libjevois module
      "sys.path.append(\"" JEVOIS_OPENCV_PYTHON_PATH "/lib\")\n" // To find cv2 module
      "sys.path.append(\"" + pydir + "\")\n" +
      "from " + m.modulename + " import " + m.modulename + "\n";
    boost::python::exec(execstr.c_str(), itsMainNamespace, itsMainNamespace);

    // Create an instance of the python class defined in the module:
    itsInstance = boost::python::eval((m.modulename + "()").c_str(), itsMainNamespace, itsMainNamespace);
  }
  catch (...) { jevois::warnAndRethrowException(); }
}

// ####################################################################################################
jevois::PythonModule::~PythonModule()
{ }

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe, OutputFrame && outframe)
{
  jevois::InputFramePython inframepy(&inframe);
  jevois::OutputFramePython outframepy(&outframe);
  itsInstance.attr("process")(boost::ref(inframepy), boost::ref(outframepy));
}

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe)
{
  jevois::InputFramePython inframepy(&inframe);
  itsInstance.attr("process")(boost::ref(inframepy));
}

// ####################################################################################################
void jevois::PythonModule::parseSerial(std::string const & str, std::shared_ptr<UserInterface> s)
{
  boost::python::object ret = itsInstance.attr("parseSerial")(str);
  std::string retstr = boost::python::extract<std::string>(ret);
  if (retstr.empty() == false) s->writeString(retstr);
}

// ####################################################################################################
void jevois::PythonModule::supportedCommands(std::ostream & os)
{
  boost::python::object ret = itsInstance.attr("supportedCommands")();
  std::string retstr = boost::python::extract<std::string>(ret);
  if (retstr.empty() == false) os << retstr;
}

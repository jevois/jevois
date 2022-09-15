// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 20122by Laurent Itti, the University of Southern
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

#include <jevois/Core/PythonParameter.H>
#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/Engine.H>

// ####################################################################################################
jevois::python::PyParHelperBase::PyParHelperBase(jevois::Component * comp) : itsComp(comp)
{ }

// ####################################################################################################
jevois::python::PyParHelperBase::~PyParHelperBase()
{ }

// ####################################################################################################
jevois::Component * jevois::python::PyParHelperBase::comp() const
{ return itsComp; }

// ####################################################################################################
// ####################################################################################################
namespace
{
  // Function signature for PyParHelper<T>::create
  using padp = std::shared_ptr<jevois::python::PyParHelperBase>(*)(jevois::Component *, std::string const &,
                                                                   std::string const &, boost::python::object const &,
                                                                   jevois::ParameterCategory const &);
  
  static padp padp_bool = jevois::python::PyParHelper<bool>::create;
  static padp padp_char = jevois::python::PyParHelper<char>::create;
  static padp padp_byte = jevois::python::PyParHelper<uint8_t>::create;
  static padp padp_int16 = jevois::python::PyParHelper<int16_t>::create;
  static padp padp_uint16 = jevois::python::PyParHelper<uint16_t>::create;
  static padp padp_int = jevois::python::PyParHelper<int32_t>::create;
  static padp padp_uint = jevois::python::PyParHelper<uint32_t>::create;
  static padp padp_long = jevois::python::PyParHelper<int64_t>::create;
  static padp padp_ulong = jevois::python::PyParHelper<uint64_t>::create;
  static padp padp_float = jevois::python::PyParHelper<float>::create;
  static padp padp_double = jevois::python::PyParHelper<double>::create;
  static padp padp_string = jevois::python::PyParHelper<std::string>::create;
  static padp padp_ipoint = jevois::python::PyParHelper<cv::Point_<int>>::create;
  static padp padp_fpoint = jevois::python::PyParHelper<cv::Point_<float>>::create;
  static padp padp_isize = jevois::python::PyParHelper<cv::Size_<int>>::create;
  static padp padp_fsize = jevois::python::PyParHelper<cv::Size_<float>>::create;
  static padp padp_iscalar = jevois::python::PyParHelper<cv::Scalar_<int>>::create;
  static padp padp_fscalar = jevois::python::PyParHelper<cv::Scalar_<float>>::create;
  
#ifdef JEVOIS_PRO  
  static padp padp_color = jevois::python::PyParHelper<ImColor>::create;
#endif
  // jevois::Parameter also supports pair<T,S>, map<K,V>, vector<T>, etc in C++...

  // Now create a mapping from type names exposed to python to C++ types:
  static std::map<std::string, padp> padp_map
  {
   { "bool", padp_bool },
   { "char", padp_char },
   { "byte", padp_byte },
   { "uint8", padp_byte },
   { "int16", padp_int16 },
   { "uint16", padp_uint16 },
   { "int", padp_int },
   { "uint", padp_uint },
   { "int32", padp_int },
   { "uint32", padp_uint },
   { "long", padp_long },
   { "int64", padp_long },
   { "uint64", padp_ulong },
   { "float", padp_float },
   { "double", padp_double },
   { "str", padp_string },
   { "ipoint", padp_ipoint },
   { "fpoint", padp_fpoint },
   { "isize", padp_isize },
   { "fsize", padp_fsize },
   { "iscalar", padp_iscalar },
   { "fscalar", padp_fscalar },

#ifdef JEVOIS_PRO
   { "color", padp_color },
#endif
  };
} // anonymous namespace

// ####################################################################################################
jevois::PythonParameter::PythonParameter(boost::python::object & pyinst, std::string const & name,
                                         std::string const & typ, std::string const & description,
                                         boost::python::object const & defaultValue,
                                         jevois::ParameterCategory const & category)
{
  auto itr = padp_map.find(typ);

  if (itr == padp_map.end())
  {
    std::string errmsg = "Unsupported parameter type [" + typ + "] -- Supported types are: ";
    for (auto const & mapping : padp_map) errmsg += std::string(mapping.first) + ", ";
    LFATAL(errmsg << "as of currently running JeVois " << JEVOIS_VERSION_STRING);
  }
  
  // We add the parameter to component registered with the python code:
  jevois::Component * comp = jevois::python::engine()->getPythonComponent(pyinst.ptr()->ob_type);

  // Ok, create the parameter with the desired type:
  itsPyPar = itr->second(comp, name, description, defaultValue, category);
}

// ####################################################################################################
jevois::PythonParameter::~PythonParameter()
{ }

// ####################################################################################################
std::string const & jevois::PythonParameter::name() const
{ return itsPyPar->par()->name(); }

// ####################################################################################################
std::string jevois::PythonParameter::descriptor() const
{ return itsPyPar->par()->descriptor(); }

// ####################################################################################################
boost::python::object jevois::PythonParameter::get() const
{ return itsPyPar->get(); }

// ####################################################################################################
void jevois::PythonParameter::set(boost::python::object const & newVal)
{ itsPyPar->set(newVal); }

// ####################################################################################################
std::string const jevois::PythonParameter::strget() const
{ return itsPyPar->par()->strget(); }

// ####################################################################################################
void jevois::PythonParameter::strset(std::string const & valstring)
{ itsPyPar->par()->strset(valstring); }

// ####################################################################################################
void jevois::PythonParameter::freeze(bool doit)
{ itsPyPar->par()->freeze(doit); }

// ####################################################################################################
void jevois::PythonParameter::reset()
{ itsPyPar->par()->reset(); }

// ####################################################################################################
void jevois::PythonParameter::setCallback(boost::python::object const & cb)
{ itsPyPar->setCallback(cb); }

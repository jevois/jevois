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

// This code is inspired by the Neuromorphic Robotics Toolkit (http://nrtkit.org)

#pragma once

#include <jevois/Util/Utils.H>
#include <cctype>

// ######################################################################
template <typename T> inline
void jevois::paramValToString(T const & val, std::string & result)
{
  result = jevois::to_string(val);
}

// ######################################################################
template <> inline
void jevois::paramValToString<unsigned char>(unsigned char const & val, std::string & result)
{
  // Convert to int so we don't interpret val as a character:
  result = jevois::to_string(int(val));
}

// ######################################################################
template <typename T> inline
void jevois::paramStringToVal(std::string const & valstring, T & result)
{
  result = jevois::from_string<T>(valstring);
}

// ######################################################################
template <typename Q> inline
void jevois::paramValToString(std::vector<Q> const & val, std::string & result) 
{  
  std::stringstream s; s << val; result = s.str();
}

// ######################################################################
template <typename Q> inline
void jevois::paramStringToVal(std::string const & valstring, std::vector<Q> & result) 
{
  std::stringstream s(valstring); s >> result;
}

// ######################################################################
template <typename F, typename S> inline
void jevois::paramValToString(std::pair<F, S> const & val, std::string & result) 
{  
  std::stringstream s; s << val.first << ' ' << val.second; result = s.str();
}

// ######################################################################
template <typename F, typename S> inline
void jevois::paramStringToVal(std::string const & valstring, std::pair<F, S> & result) 
{
  std::stringstream s(valstring); s >> result.first >> std::skipws >> result.second;
}

// ######################################################################
template <typename T> inline
void jevois::paramValToString(cv::Point_<T> const & val, std::string & result)
{
  std::stringstream s; s << val.x << ' ' << val.y; result = s.str();
}

// ######################################################################
template <typename T> inline
void jevois::paramStringToVal(std::string const & valstring, cv::Point_<T> & result)
{
  std::stringstream s(valstring); s >> result.x >> std::skipws >> result.y;
}

// ######################################################################
template <typename T> inline
void jevois::paramValToString(cv::Size_<T> const & val, std::string & result)
{
  std::stringstream s; s << val.width << ' ' << val.height; result = s.str();
}

// ######################################################################
template <typename T> inline
void jevois::paramStringToVal(std::string const & valstring, cv::Size_<T> & result)
{
  std::stringstream s(valstring); s >> result.width >> std::skipws >> result.height;
}

// ######################################################################
template <typename K, typename V> inline
void jevois::paramValToString(std::map<K,V> const & val, std::string & result) 
{  
  std::stringstream s; s << val; result = s.str();
}

// ######################################################################
template <typename K, typename V> inline
void jevois::paramStringToVal(std::string const & valstring, std::map<K,V> & result) 
{
  std::stringstream s(valstring); s >> result;
}

// ######################################################################
template <> inline
void jevois::paramValToString<bool>(bool const & val, std::string & result)
{
  if (val) result = "true"; else result = "false";
}

// ######################################################################
template <> inline
void jevois::paramStringToVal<bool>(std::string const & valstring, bool & result)
{
  std::string vs = valstring; for (char & c : vs) c = std::tolower(c);

  if (vs == "" || vs == "true" || vs == "t" || vs == "yes" || vs == "y" || vs == "1") result = true;
  else if (vs == "false" || vs == "f" || vs == "no" || vs == "n" || vs == "0") result = false;
  else result = jevois::from_string<bool>(valstring);
}

// ######################################################################
template <> inline
void jevois::paramValToString<std::string>(std::string const & val, std::string & result)
{ result = val; }

// ######################################################################
template <> inline
void jevois::paramStringToVal<std::string>(std::string const & valstring, std::string & result)
{ result = valstring; }


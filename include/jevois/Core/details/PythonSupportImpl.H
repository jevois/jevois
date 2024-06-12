// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2022 by Laurent Itti, the University of Southern
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

#pragma once

// ####################################################################################################
template <class T> inline
boost::python::list jevois::python::pyVecToList(std::vector<T> const & v)
{
  boost::python::list ret;
  for (T const & elem : v) ret.append(elem);
  return ret;
}

// ####################################################################################################
template <class T> inline
std::vector<T> jevois::python::pyListToVec(boost::python::object const & lst)
{
  return std::vector<T>(boost::python::stl_input_iterator<T>(lst), boost::python::stl_input_iterator<T>());
}

// ####################################################################################################
template <typename T, size_t N> inline
std::array<T, N> jevois::python::tupleToArray(boost::python::object const & o)
{

  std::array<T, N> arr;
  jevois::python::tupleToArray(o, arr, N);
  return arr;
}

// ####################################################################################################
template <typename T, size_t N> inline
void jevois::python::tupleToArray(boost::python::object const & o, std::array<T, N> & arr, size_t minN)
{
  boost::python::tuple tup = boost::python::extract<boost::python::tuple>(o);
  size_t n = boost::python::len(tup);

  if (n > N || n < minN)
    throw std::range_error("Received " + std::to_string(n) + "-element tuple, need " +
                           std::to_string(minN) + ".." + std::to_string(N));

  for (size_t i = 0; i < n; ++i) arr[i] = boost::python::extract<T>(tup[i]);
}

// ####################################################################################################
template <typename T> inline
T jevois::python::pyextract(boost::python::object const & o)
{
#ifdef JEVOIS_PLATFORM

  // False alarm warning on g++ on platform
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  return boost::python::extract<T>(o);
#pragma GCC diagnostic pop

#else
  
  return boost::python::extract<T>(o);

#endif
}

// ####################################################################################################
template <> inline
cv::Scalar_<float> jevois::python::pyextract<cv::Scalar_<float>>(boost::python::object const & o)
{
  std::array<float, 4> arr { };
  jevois::python::tupleToArray(o, arr, 1);
  return cv::Scalar_<float>(arr[0], arr[1], arr[2], arr[3]);
}
  
// ####################################################################################################
template <> inline
cv::Scalar_<int> jevois::python::pyextract<cv::Scalar_<int>>(boost::python::object const & o)
{
  std::array<int, 4> arr { };
  jevois::python::tupleToArray(o, arr, 1);
  return cv::Scalar_<int>(arr[0], arr[1], arr[2], arr[3]);
}
  
// ####################################################################################################
template <> inline
cv::Point_<float> jevois::python::pyextract<cv::Point_<float>>(boost::python::object const & o)
{
  std::array<float, 2> arr { };
  jevois::python::tupleToArray(o, arr, 2);
  return cv::Point_<float>(arr[0], arr[1]);
}
  
// ####################################################################################################
template <> inline
cv::Point_<int> jevois::python::pyextract<cv::Point_<int>>(boost::python::object const & o)
{
  std::array<int, 2> arr { };
  jevois::python::tupleToArray(o, arr, 2);
  return cv::Point_<int>(arr[0], arr[1]);
}

// ####################################################################################################
template <> inline
cv::Size_<float> jevois::python::pyextract<cv::Size_<float>>(boost::python::object const & o)
{
  std::array<float, 2> arr { };
  jevois::python::tupleToArray(o, arr, 2);
  return cv::Size_<float>(arr[0], arr[1]);
}
  
// ####################################################################################################
template <> inline
cv::Size_<int> jevois::python::pyextract<cv::Size_<int>>(boost::python::object const & o)
{
  std::array<int, 2> arr { };
  jevois::python::tupleToArray(o, arr, 2);
  return cv::Size_<int>(arr[0], arr[1]);
}

// ####################################################################################################
#ifdef JEVOIS_PRO  
template <> inline
ImColor jevois::python::pyextract<ImColor>(boost::python::object const & o)
{
  std::array<int, 4> arr { 0, 0, 0, 255 };
  jevois::python::tupleToArray(o, arr, 3);
  return ImColor(arr[0], arr[1], arr[2], arr[3]);
}
#endif

// ####################################################################################################
template <typename T> inline
boost::python::object jevois::python::topyobj(T const & val)
{ return boost::python::object(val); }

// ####################################################################################################
template <> inline
boost::python::object jevois::python::topyobj(cv::Scalar_<float> const & val)
{ return boost::python::make_tuple(val[0], val[1], val[2], val[3]); }

// ####################################################################################################
template <> inline
boost::python::object jevois::python::topyobj(cv::Scalar_<int> const & val)
{ return boost::python::make_tuple(val[0], val[1], val[2], val[3]); }

// ####################################################################################################
template <> inline
boost::python::object jevois::python::topyobj(cv::Point_<float> const & val)
{ return boost::python::make_tuple(val.x, val.y); }

// ####################################################################################################
template <> inline
boost::python::object jevois::python::topyobj(cv::Point_<int> const & val)
{ return boost::python::make_tuple(val.x, val.y); }

// ####################################################################################################
template <> inline
boost::python::object jevois::python::topyobj(cv::Size_<float> const & val)
{ return boost::python::make_tuple(val.width, val.height); }

// ####################################################################################################
template <> inline
boost::python::object jevois::python::topyobj(cv::Size_<int> const & val)
{ return boost::python::make_tuple(val.width, val.height); }

// ####################################################################################################
#ifdef JEVOIS_PRO  
template <> inline
boost::python::object jevois::python::topyobj(ImColor const & val)
{ return boost::python::make_tuple(int(val.Value.x * 255.0F), int(val.Value.y * 255.0F),
                                   int(val.Value.z * 255.0F), int(val.Value.w * 255.0F)); }
#endif

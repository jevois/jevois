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

// This file has been modified / imported from the Neuromorphic Robotics Toolkit (NRT). Original copyright is:

/* author Laurent Itti
    // ////////////////////////////////////////////////////////////////////////
    //              The iLab Neuromorphic Robotics Toolkit (NRT)             //
    // Copyright 2010-2012 by the University of Southern California (USC)    //
    //                          and the iLab at USC.                         //
    //                                                                       //
    //                iLab - University of Southern California               //
    //                Hedco Neurociences Building, Room HNB-10               //
    //                    Los Angeles, Ca 90089-2520 - USA                   //
    //                                                                       //
    //      See http://ilab.usc.edu for information about this project.      //
    // ////////////////////////////////////////////////////////////////////////
    // This file is part of The iLab Neuromorphic Robotics Toolkit.          //
    //                                                                       //
    // The iLab Neuromorphic Robotics Toolkit is free software: you can      //
    // redistribute it and/or modify it under the terms of the GNU General   //
    // Public License as published by the Free Software Foundation, either   //
    // version 3 of the License, or (at your option) any later version.      //
    //                                                                       //
    // The iLab Neuromorphic Robotics Toolkit is distributed in the hope     //
    // that it will be useful, but WITHOUT ANY WARRANTY; without even the    //
    // implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       //
    // PURPOSE.  See the GNU General Public License for more details.        //
    //                                                                       //
    // You should have received a copy of the GNU General Public License     //
    // along with The iLab Neuromorphic Robotics Toolkit.  If not, see       //
    // <http://www.gnu.org/licenses/>.                                       //
    // ////////////////////////////////////////////////////////////////////////
*/

#pragma once

#include <jevois/Component/ParameterStringConversion.H>
#include <jevois/Util/Utils.H>

// ######################################################################
// Inline function implementations
// ######################################################################
template <class T> inline
jevois::Range<T>::Range() :
    itsMin(0), itsMax(0)
{ }

// ######################################################################
template <class T> inline
jevois::Range<T>::Range(T const mini, T const maxi) :
    itsMin(mini), itsMax(maxi)
{
  if (itsMax < itsMin) std::swap(itsMin, itsMax);
}

// ######################################################################
template <class T> template <class U> inline
jevois::Range<T>::Range(jevois::Range<U> const & other) :
    itsMin(jevois::clamped_convert<T, U>(other.min())),
    itsMax(jevois::clamped_convert<T, U>(other.max()))
{ }

// ######################################################################
template <class T> inline
T const & jevois::Range<T>::min() const
{ return itsMin; }

// ######################################################################
template <class T> inline
T const & jevois::Range<T>::max() const
{ return itsMax; }

// ######################################################################
template <class T> inline
bool jevois::Range<T>::empty() const
{ return (itsMax == itsMin); }


// ######################################################################
template <class T> inline
void jevois::Range<T>::extend(T const val)
{
  if (itsMax < val) itsMax = val;
  if (val < itsMin) itsMin = val;
}

// ######################################################################
template <class T> inline
bool jevois::Range<T>::contains(T const & val) const
{ return (val >= itsMin && val <= itsMax); }

// ######################################################################
// Free function implementations
// ######################################################################
template <class T> inline
jevois::Range<T> jevois::merge(jevois::Range<T> const & r1, jevois::Range<T> const & r2)
{
  return jevois::Range<T>(std::min(r1.min(), r2.min()), std::max(r1.max(), r2.max()));
}

// ######################################################################
template <class T> inline
std::ostream & jevois::operator<<(std::ostream & out, jevois::Range<T> const & r)
{ return out << '[' << r.min() << " ... " << r.max() << ']'; }

// ######################################################################
template <class T> inline
std::istream & jevois::operator>>(std::istream & in, jevois::Range<T> & r)
{
  char c; do { in >> c; } while (std::isspace(c));
  if (c != '[') throw std::range_error("Error parsing Range: missing '['");
  T mi; in >> mi; if (in.fail()) throw std::range_error("Error parsing Range: bad min");
  do { in >> c; } while (std::isspace(c));
  for (int i = 0; i < 3; ++i)
  {
    if (c != '.') throw std::range_error("Error parsing Range: missing '...'");
    in >> c;
  }
  T ma; in >> ma; if (in.fail()) throw std::range_error("Error parsing Range: bad max");
  do { in >> c; } while (std::isspace(c));
  if (c != ']') throw std::range_error("Error parsing Range: missing ']'");

  r = jevois::Range<T>(mi, ma);

  return in;
}

// ######################################################################
template <class T> inline
void jevois::paramValToString(jevois::Range<T> const & val, std::string & result)
{
  // Use paramValToString on T so we correctly handle unsigned char:
  std::string mi; jevois::paramValToString(val.min(), mi);
  std::string ma; jevois::paramValToString(val.max(), ma);
  result = mi + "..." + ma;
}

// ######################################################################
template <class T> inline
void jevois::paramStringToVal(std::string const & valstring, jevois::Range<T> & result)
{
  std::vector<std::string> splitVec = jevois::split(valstring, "\\.\\.\\.");

  if (splitVec.size() != 2) throw std::range_error("Error parsing Range from [" + valstring + "]: missing '...'");

  T mi = 0, ma = 0;
  try { jevois::paramStringToVal(splitVec[0], mi); }
  catch (...) { throw std::range_error("Error parsing Range from [" + valstring + "]: invalid min value"); }

  try { jevois::paramStringToVal(splitVec[1], ma); }
  catch (...) { throw std::range_error("Error parsing Range from [" + valstring + "]: invalid max value"); }

  result = jevois::Range<T>(mi, ma);
}

// ######################################################################
template <class T> inline
bool jevois::operator==(jevois::Range<T> const & range1, jevois::Range<T> const & range2)
{ return (range1.min() == range2.min()) && (range1.max() == range2.max()); }

// ######################################################################
template <class T> inline
bool jevois::operator!=(jevois::Range<T> const & range1, jevois::Range<T> const & range2)
{ return (range1.min() != range2.min()) || (range1.max() != range2.max()); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator+(jevois::Range<T> const & range, T const & scalar)
{ return jevois::Range<T>(range.min() + scalar, range.max() + scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator+(T const & scalar, jevois::Range<T> const & range)
{ return jevois::Range<T>(scalar + range.min(), scalar + range.max()); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator-(jevois::Range<T> const & range, T const & scalar)
{ return jevois::Range<T>(range.min() - scalar, range.max() - scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator-(T const & scalar, jevois::Range<T> const & range)
{ return jevois::Range<T>(scalar - range.min(), scalar - range.max()); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator/(jevois::Range<T> const & range, T const & scalar)
{ return jevois::Range<T>(range.min() / scalar, range.max() / scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator/(T const & scalar, jevois::Range<T> const & range)
{ return jevois::Range<T>(scalar / range.min(), scalar / range.max()); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator*(jevois::Range<T> const & range, T const & scalar)
{ return jevois::Range<T>(range.min() * scalar, range.max() * scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> jevois::operator*(T const & scalar, jevois::Range<T> const & range)
{ return jevois::Range<T>(scalar * range.min(), scalar * range.max()); }

// ######################################################################
template <class T> inline
jevois::Range<T> & jevois::operator+=(jevois::Range<T> & range, T const & scalar)
{ return range = jevois::Range<T>(range.min() + scalar, range.max() + scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> & jevois::operator-=(jevois::Range<T> & range, T const & scalar)
{ return range = jevois::Range<T>(range.min() - scalar, range.max() - scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> & jevois::operator*=(jevois::Range<T> & range, T const & scalar)
{ return range = jevois::Range<T>(range.min() * scalar, range.max() * scalar); }

// ######################################################################
template <class T> inline
jevois::Range<T> & jevois::operator/=(jevois::Range<T> & range, T const & scalar)
{ return range = jevois::Range<T>(range.min() / scalar, range.max() / scalar); }


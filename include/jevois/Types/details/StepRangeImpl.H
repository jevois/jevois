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
template <typename T> inline
jevois::StepRange<T>::StepRange() :
    itsMin(0), itsStep(0), itsMax(0)
{ }

// ######################################################################
template <typename T> inline
jevois::StepRange<T>::StepRange(T const mini, T const stepi, T const maxi) :
    itsMin(mini), itsStep(stepi), itsMax(maxi)
{
  if (itsStep < 0) throw std::range_error("Invalid negative step value given to StepRange (must be >= 0)");
  if (itsMax < itsMin) std::swap(itsMin, itsMax);
}

// ######################################################################
template <typename T> template <typename U> inline
jevois::StepRange<T>::StepRange(jevois::StepRange<U> const & other) :
    itsMin(jevois::clamped_convert<T, U>(other.min())),
    itsStep(jevois::clamped_convert<T, U>(other.step())),
    itsMax(jevois::clamped_convert<T, U>(other.max()))
{ }

// ######################################################################
template <typename T> inline
T const & jevois::StepRange<T>::min() const
{ return itsMin; }

// ######################################################################
template <typename T> inline
T const & jevois::StepRange<T>::step() const
{ return itsStep; }

// ######################################################################
template <typename T> inline
T const & jevois::StepRange<T>::max() const
{ return itsMax; }

// ######################################################################
template <typename T> inline
bool jevois::StepRange<T>::empty() const
{ return (itsMax == itsMin); }


// ######################################################################
template <typename T> inline
bool jevois::StepRange<T>::isValueValid(T const val) const
{
  if (val < itsMin || val > itsMax) return false;
  if (val == itsMax) return true;
  if (itsStep == 0) return true;

  double n = (static_cast<double>(val) - static_cast<double>(itsMin)) / static_cast<double>(itsStep);

  if (n - int(n) > 1.0e-10) return false;

  return true;
}

// ######################################################################
// Free function implementations
// ######################################################################

// ######################################################################
template <typename T> inline
std::ostream & jevois::operator<<(std::ostream & out, jevois::StepRange<T> const & r)
{ return out << '[' << r.min() << " ...(" << r.step() << ")... " << r.max() << ']'; }

// ######################################################################
template <typename T> inline
std::istream & jevois::operator>>(std::istream & in, jevois::StepRange<T> & r)
{
  char c; do { in >> c; } while (std::isspace(c));
  if (c != '[') throw std::range_error("Error parsing StepRange: missing '['");

  T mi; in >> mi; if (in.fail()) throw std::range_error("Error parsing StepRange: bad min");

  do { in >> c; } while (std::isspace(c));
  for (int i = 0; i < 3; ++i)
  {
    if (c != '.')  throw std::range_error("Error parsing StepRange: missing first '...'");
    in >> c;
  }

  if (c != '(') throw std::range_error("Error parsing StepRange: missing '('");
  T st; in >> st; if (in.fail()) throw std::range_error("Error parsing StepRange: bad step");
  in >> c;
  if (c != ')') throw std::range_error("Error parsing StepRange: missing '('");
  
  for (int i = 0; i < 3; ++i)
  {
    in >> c;
    if (c != '.')  throw std::range_error("Error parsing StepRange: missing second '...'");
  }

  T ma; in >> ma; if (in.fail()) throw std::range_error("Error parsing StepRange: bad max");

  do { in >> c; } while (std::isspace(c));
  if (c != ']') throw std::range_error("Error parsing StepRange: missing ']'");

  r = jevois::StepRange<T>(mi, st, ma);

  return in;
}

// ######################################################################
template <class T> inline
void jevois::paramValToString(jevois::StepRange<T> const & val, std::string & result)
{
  // Use paramValToString on T so we correctly handle unsigned char:
  std::string mi; jevois::paramValToString(val.min(), mi);
  std::string st; jevois::paramValToString(val.step(), st);
  std::string ma; jevois::paramValToString(val.max(), ma);
  result = mi + "..." + st + "..." + ma;
}

// ######################################################################
template <class T> inline
void jevois::paramStringToVal(std::string const & valstring, jevois::StepRange<T> & result)
{
  std::vector<std::string> splitVec = jevois::split(valstring, "\\.\\.\\.");

  if (splitVec.size() != 3) throw std::range_error("Error parsing StepRange from [" + valstring + "]: missing '...'");

  T mi = 0, st = 0, ma = 0;
  try { jevois::paramStringToVal(splitVec[0], mi); }
  catch (...) { throw std::range_error("Error parsing StepRange from [" + valstring + "]: invalid min value"); }

  try { jevois::paramStringToVal(splitVec[1], st); }
  catch (...) { throw std::range_error("Error parsing StepRange from [" + valstring + "]: invalid step value"); }

  try { jevois::paramStringToVal(splitVec[2], ma); }
  catch (...) { throw std::range_error("Error parsing StepRange from [" + valstring + "]: invalid max value"); }

  result = jevois::StepRange<T>(mi, st, ma);
}

// ######################################################################
template <typename T> inline
bool jevois::operator==(jevois::StepRange<T> const & range1, jevois::StepRange<T> const & range2)
{ return (range1.min() == range2.min()) && (range1.step() == range2.step()) && (range1.max() == range2.max()); }

// ######################################################################
template <typename T> inline
bool jevois::operator!=(jevois::StepRange<T> const & range1, jevois::StepRange<T> const & range2)
{ return (range1.min() != range2.min()) || (range1.step() != range2.step()) || (range1.max() != range2.max()); }


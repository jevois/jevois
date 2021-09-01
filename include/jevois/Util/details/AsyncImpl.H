// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2021 by Laurent Itti, the University of Southern
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

#ifdef JEVOIS_PRO

#include <jevois/Util/ThreadPool.H>

// ####################################################################################################
#ifdef JEVOIS_PLATFORM
namespace jevois
{
  namespace details
  {
    extern jevois::ThreadPool ThreadpoolLittle;
    extern jevois::ThreadPool ThreadpoolBig;
  }
}

template <class Function, class... Args>
[[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
jevois::async(Function && f, Args &&... args)
{
  return jevois::details::ThreadpoolBig.execute(std::forward<Function>(f), std::forward<Args>(args)...);
}

template <class Function, class... Args>
[[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
jevois::async_little(Function && f, Args &&... args)
{
  return jevois::details::ThreadpoolLittle.execute(std::forward<Function>(f), std::forward<Args>(args)...);
}

// ####################################################################################################
#else // JEVOIS_PLATFORM
namespace jevois
{
  namespace details
  {
    extern jevois::ThreadPool Threadpool;
  }
}

template <class Function, class... Args>
[[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
jevois::async(Function && f, Args &&... args)
{
  return jevois::details::Threadpool.execute(std::forward<Function>(f), std::forward<Args>(args)...);
}

template <class Function, class... Args>
[[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
jevois::async_little(Function && f, Args &&... args)
{
  return jevois::details::Threadpool.execute(std::forward<Function>(f), std::forward<Args>(args)...);
}
#endif // JEVOIS_PLATFORM

// ####################################################################################################

#else // JEVOIS_PRO

// Do not use a thread pool on JeVois-A33, just std::async:
#include <future>

template <class Function, class... Args>
[[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
jevois::async(Function && f, Args &&... args)
{
  return std::async(std::launch::async, std::forward<Function>(f), std::forward<Args>(args)...);
}

template <class Function, class... Args>
[[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
jevois::async_little(Function && f, Args &&... args)
{
  return std::async(std::launch::async, std::forward<Function>(f), std::forward<Args>(args)...);
}

#endif // JEVOIS_PRO

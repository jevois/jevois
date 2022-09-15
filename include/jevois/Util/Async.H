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

#include <future>
#include <vector>

namespace jevois
{
  //! Async execution using a thread pool
  /*! Same function signature and usage as std::async(). Always launches the task in a parallel thread, using a thread
      pool. Runs the task on big (ARM A73) cores when using big.little CPU architecture of JeVois Pro Platform. Use this
      function to launch parallel threads of high priority, e.g., machine vision algorithms. \ingroup utils */
  template <class Function, class... Args>
  [[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
  async(Function && f, Args &&... args);

  //! Async execution using a thread pool
  /*! Same function signature and usage as std::async(). Always launches the task in a parallel thread, using a thread
      pool. Runs the task on little (ARM A53) cores when using big.little CPU architecture of JeVois Pro Platform, or on
      the same thread pool as jevois::async() on other architectures. Use this function to run threads that are not very
      compute intensive, e.g., threads that may sleep most of the time until some condition has become
      satisfied. \ingroup utils */
  template <class Function, class... Args>
  [[nodiscard]] std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
  async_little(Function && f, Args &&... args);

  //! Collect results from several async threads that are all returning a T result
  /*! Guarantees that get() will run on all futures. If any throws, collect all the error messages (if possible, e.g.,
      an std::exception was thrown) and assemble them into a single string of the form "[error 1][error 2] ..." or
      "error 1\nerror 2\n...", then throw that as an std::runtime_error \ingroup utils */
  template <typename T>
  std::vector<T> joinall(std::vector<std::future<T>> & fvec, bool multiline = true);

  //! Collect results from several async threads that are all returning a T result
  /*! Guarantees that get() will run on all futures. If any throws, collect all the error messages (if possible, e.g.,
      an std::exception was thrown) and assemble them into a single string of the form "[error 1][error 2] ..." or
      "error 1\nerror 2\n...", then throw that as an std::runtime_error \ingroup utils */
  void joinall(std::vector<std::future<void>> & fvec, bool multiline = true);
  
} // namespace jevois

// Include implementation details:
#include <jevois/Util/details/AsyncImpl.H>

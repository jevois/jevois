// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

// This code modified from:

//          Copyright Mateusz Jaworski 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <future>
#include <thread>
#include <vector>
#include <stdexcept>
#include <type_traits>

#include "function2/function2.hpp"
#include "concurrentqueue/concurrentqueue.h"

#include <sched.h>

namespace jevois
{
  //! A thread pool with CPU affinity
  /*! Thread pool with extra settings to enforce that passed tasks run on some particular cores. Used by JeVois-Pro
      Platform to enforce that machine vision tasks run on big cores (ARM A73) while non-critical tasks run on slower
      little cores (ARM A53).  \ingroup utils */
  class ThreadPool
  {
    public:
      //! Constructor
      ThreadPool(unsigned int threads = std::thread::hardware_concurrency(), bool little = false);

      //! Destructor
      ~ThreadPool();

      //! Execute a function and get a future
      template<typename Func, typename... Args, std::enable_if_t<std::is_invocable_v<Func&&, Args&&...>, bool> = true>
      auto execute(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;

      //! Get the pool size
      auto getPoolSize() -> size_t;

    private:
      ThreadPool& operator=(ThreadPool&) = delete;
      ThreadPool(ThreadPool&) = delete;
      moodycamel::ConcurrentQueue<fu2::unique_function<void()>> _tasks;
      std::atomic<unsigned int> _size;
      std::vector<std::thread> _pool;
      std::condition_variable _new_task;
      std::mutex _mtx;
      bool _exit;
  };

}

// Include implementation details
#include <jevois/Util/details/ThreadPoolImpl.H>

#endif // JEVOIS_PRO

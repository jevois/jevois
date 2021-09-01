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

template<typename Func, typename... Args, std::enable_if_t<std::is_invocable_v<Func&&, Args&&...>, bool> >
[[nodiscard]] auto jevois::ThreadPool::execute(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
{
  auto task = std::packaged_task<decltype(func(args...))()>
    ([func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable
     { return func(std::forward<Args>(args)...); });

  auto ret =  task.get_future();
  
  {
    std::scoped_lock<std::mutex> lock(_mtx);
    
    if (!_tasks.enqueue([task = std::move(task)]() mutable { task(); }))
      throw std::runtime_error("Could not queue up a task");
    _size++;
  }
  
  _new_task.notify_one();
  return ret;
}

#endif // JEVOIS_PRO

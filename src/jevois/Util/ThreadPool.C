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

#ifdef JEVOIS_PRO

// This code modified from:

//          Copyright Mateusz Jaworski 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <jevois/Util/ThreadPool.H>
#include <jevois/Util/Async.H>
#include <jevois/Debug/Log.H>
#include <pthread.h>

// ##############################################################################################################
jevois::ThreadPool::ThreadPool(unsigned int threads, bool little) :
    _size(0), _exit(false)
{
  _pool.reserve(threads);
  
#ifdef JEVOIS_PLATFORM
  // Define a CPU mask:
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  
  // Little cores are 0, 1; big cores are 2,3,4,5:
  if (little)
  {
    CPU_SET(0, &cpuset);
    CPU_SET(1, &cpuset);
  }
  else
  {
    CPU_SET(2, &cpuset);
    CPU_SET(3, &cpuset);
    CPU_SET(4, &cpuset);
    CPU_SET(5, &cpuset);
  }
#endif
  
  for(unsigned int i = 0; i < threads; ++i)
  {
    _pool.emplace_back([this]{
                         fu2::unique_function<void()> task;
                         
                         for (;;)
                         {
                           {
                             std::unique_lock<std::mutex> lock(_mtx);
                             _new_task.wait(lock, [this]{ return _size || _exit; });
                           }
                           
                           //if (_exit && !_size)) return; // JEVOIS: this can lock up the destructor...

                           if (_exit)
                           {
                             while (_tasks.try_dequeue(task)) { _size--; task(); }
                             return;
                           }
                           
                           if (_tasks.try_dequeue(task))
                           {
                             _size--;
                             task();
                           }
                         }
                         
                       });
    
#ifdef JEVOIS_PLATFORM
    // JeVois: set CPU affinity for this thread:
    int rc = pthread_setaffinity_np(_pool.back().native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc) LERROR("Error calling pthread_setaffinity_np: " << rc);
#endif
  }

  // Run a phony job, otherwise the threadpool hangs on destruction if it was never used:
#ifdef JEVOIS_PLATFORM
  auto fut = execute([threads,little](){
                       LINFO("Initialized with " << threads << (little ? " A53 threads." : " A73 threads.")); });
#else
  auto fut = execute([threads](){
                       LINFO("Initialized with " << threads << " threads."); });
  (void)little; // keep compiler happy
#endif
}

// ##############################################################################################################
jevois::ThreadPool::~ThreadPool()
{
  {
    std::scoped_lock<std::mutex> lock(_mtx);
    _exit = true;
  }
  
  _new_task.notify_all();
  for (auto & thread: _pool) thread.join();
}

// ##############################################################################################################
auto jevois::ThreadPool::getPoolSize() -> size_t
{
  return _pool.size();
}

// ##############################################################################################################
// ##############################################################################################################
// ##############################################################################################################
jevois::ParallelForAPIjevois::ParallelForAPIjevois(jevois::ThreadPool * tp) : itsThreadpool(tp), itsNumThreads(4)
{ }

// ##############################################################################################################
jevois::ParallelForAPIjevois::~ParallelForAPIjevois()
{ }

// ##############################################################################################################
void jevois::ParallelForAPIjevois::parallel_for(int tasks,
                                                cv::parallel::ParallelForAPI::FN_parallel_for_body_cb_t body_callback,
                                                void * callback_data)
{
  LDEBUG("Called with " << tasks << " tasks");
  
  // Dispatch several groups of parallel threads; assumes all tasks take the same time...
  for (int group = 0; group < tasks; group += itsNumThreads)
  {
    std::vector<std::future<void>> fvec;
    int const last = std::min(tasks, group + itsNumThreads);

    for (int i = group; i < last; ++i)
      fvec.emplace_back(itsThreadpool->execute(body_callback, i, i+1, callback_data));

    // Wait until all tasks done, throw a single exception if any task threw:
    jevois::joinall(fvec);
  }
}

// ##############################################################################################################
int jevois::ParallelForAPIjevois::getThreadNum() const
{
  return (int)(size_t)(void*)pthread_self(); // no zero-based indexing
}

// ##############################################################################################################
int jevois::ParallelForAPIjevois::getNumThreads() const
{ return itsNumThreads; }

// ##############################################################################################################
int jevois::ParallelForAPIjevois::setNumThreads(int nThreads)
{
  if (nThreads != 4) LERROR("Only 4 threads supported -- IGNORED request to set " << nThreads);
  itsNumThreads = nThreads;
  return nThreads;
}

// ##############################################################################################################
char const * jevois::ParallelForAPIjevois::getName() const
{ return "jevois"; }

#endif // JEVOIS_PRO


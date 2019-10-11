#pragma once

/*
MIT License
Copyright (c) 2019 Arlen Keshabyan (arlen.albert@gmail.com)
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <string>
#include <cstdint>

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
    #define WINDOWS_OS
#endif

#ifdef WINDOWS_OS
	#include <windows.h>
#else
	#include <pthread.h>
#endif

#ifndef WINDOWS_OS
	#include <sys/mman.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/time.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
	#include <limits.h>

	#include <cstring>

	#ifdef __APPLE__
		#include <mach/clock.h>
		#include <mach/mach.h>

		#ifndef SHM_NAME_MAX
			#define SHM_NAME_MAX 31
		#endif
	#else
		#ifndef SHM_NAME_MAX
			#define SHM_NAME_MAX 255
		#endif
	#endif
#endif

#include <cstdio>

namespace nstd::thread
{
#ifdef WINDOWS_OS
	using thread_id_type = DWORD;
#else
	using thread_id_type = pthread_t;
#endif

#ifndef WINDOWS_OS
    int __csgx__clock_get_time_realtime(struct timespec *ts)
    {
#ifdef __APPLE__
        clock_serv_t cclock;
        mach_timespec_t mts;

        if (host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock) != KERN_SUCCESS)  return -1;
        if (clock_get_time(cclock, &mts) != KERN_SUCCESS)  return -1;
        if (mach_port_deallocate(mach_task_self(), cclock) != KERN_SUCCESS)  return -1;

        ts->tv_sec = mts.tv_sec;
        ts->tv_nsec = mts.tv_nsec;

        return 0;
#else
        return clock_gettime(CLOCK_REALTIME, ts);
#endif
    }
#endif

	class helper
	{
	public:
		static inline constexpr const uint32_t WaitInfinitely = 0xffffffff;

		static thread_id_type get_current_thread_id()
        {
#ifdef WINDOWS_OS
            return ::GetCurrentThreadId();
#else
            return pthread_self();
#endif
        }

		static uint64_t get_unix_microsecond_time()
        {
#ifdef WINDOWS_OS
            FILETIME temp_time;
            ULARGE_INTEGER temp_time2;
            uint64_t result;

            ::GetSystemTimeAsFileTime(&temp_time);
            temp_time2.HighPart = temp_time.dwHighDateTime;
            temp_time2.LowPart = temp_time.dwLowDateTime;
            result = temp_time2.QuadPart;

            result = (result / 10) - (uint64_t)11644473600000000ULL;

            return result;
#else
            struct timeval temp_time;

            if (gettimeofday(&temp_time, NULL))  return 0;

            return (uint64_t)((uint64_t)temp_time.tv_sec * (uint64_t)1000000 + (uint64_t)temp_time.tv_usec);
#endif
        }

#ifndef WINDOWS_OS
		static size_t get_unix_system_alignment_size()
        {
            struct { int _int; } x;
            struct { int _int; char _char; } y;

            return sizeof(y) - sizeof(x);
        }

		static size_t align_unix_size(size_t size)
        {
            size_t align_size = get_unix_system_alignment_size();

            if (size % align_size)  size += align_size - (size % align_size);

            return size;
        }

		static int init_unix_named_mem(char *&result_mem, size_t &start_pos, const char *prefix, const char *name, size_t size)
        {
            int result{ -1 };
            result_mem = nullptr;
            start_pos = (name != nullptr ? align_unix_size(1) + align_unix_size(sizeof(pthread_mutex_t)) + align_unix_size(sizeof(uint32_t)) : 0);

            size += start_pos;
            size = align_unix_size(size);

            if (name == nullptr)
            {
                result_mem = new char[size];

                result = 0;
            }
            else
            {
                char name2[SHM_NAME_MAX], nums[50];
                size_t x, x2 = 0, y = strlen(prefix), z = 0;

                memset(name2, 0, sizeof(name2));

                for (x = 0; x < y; x++)
                {
                    name2[x2] = (char)(((unsigned int)(unsigned char)name2[x2]) * 37 + ((unsigned int)(unsigned char)prefix[x]));
                    x2++;

                    if (x2 == sizeof(name2) - 1)
                    {
                        x2 = 1;
                        z++;
                    }
                }

                sprintf(nums, "-%u-%u-", (unsigned int)get_unix_system_alignment_size(), (unsigned int)size);

                y = strlen(nums);
                for (x = 0; x < y; x++)
                {
                    name2[x2] = (char)(((unsigned int)(unsigned char)name2[x2]) * 37 + ((unsigned int)(unsigned char)nums[x]));
                    x2++;

                    if (x2 == sizeof(name2) - 1)
                    {
                        x2 = 1;
                        z++;
                    }
                }

                y = strlen(name);
                for (x = 0; x < y; x++)
                {
                    name2[x2] = (char)(((unsigned int)(unsigned char)name2[x2]) * 37 + ((unsigned int)(unsigned char)name[x]));
                    x2++;

                    if (x2 == sizeof(name2) - 1)
                    {
                        x2 = 1;
                        z++;
                    }
                }

                if (z)
                {
                    unsigned char temp_chr;
                    y = (z > 1 ? sizeof(name2) - 1 : x2);
                    for (x = 1; x < y; x++)
                    {
                        temp_chr = ((unsigned char)name2[x]) & 0x3F;

                        if (temp_chr < 10)  temp_chr += '0';
                        else if (temp_chr < 36)  temp_chr = temp_chr - 10 + 'A';
                        else if (temp_chr < 62)  temp_chr = temp_chr - 36 + 'a';
                        else if (temp_chr == 62)  temp_chr = '_';
                        else temp_chr = '-';

                        name2[x] = (char)temp_chr;
                    }
                }

                for (x = 1; x < sizeof(name2) && name2[x]; x++)
                {
                    if (name2[x] == '\\' || name2[x] == '/')  name2[x] = '_';
                }

                pthread_mutex_t *mutex_ptr;
                uint32_t *ref_count_ptr;

                mode_t prev_mask = umask(0);
                int fp = shm_open(name2, O_RDWR | O_CREAT | O_EXCL, 0666);
                if (fp > -1)
                {
                    while (ftruncate(fp, size) < 0 && errno == EINTR)
                    {
                    }

                    result_mem = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
                    if (result_mem == MAP_FAILED)  result_mem = NULL;
                    else
                    {
                        pthread_mutexattr_t mutex_attr;

                        pthread_mutexattr_init(&mutex_attr);
                        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

                        mutex_ptr = reinterpret_cast<pthread_mutex_t *>(result_mem + align_unix_size(1));
                        ref_count_ptr = reinterpret_cast<uint32_t *>(result_mem + align_unix_size(1) + align_unix_size(sizeof(pthread_mutex_t)));

                        pthread_mutex_init(mutex_ptr, &mutex_attr);
                        pthread_mutex_lock(mutex_ptr);

                        result_mem[0] = '\x01';
                        ref_count_ptr[0] = 1;

                        result = 0;
                    }

                    close(fp);
                }
                else
                {
                    fp = shm_open(name2, O_RDWR, 0666);
                    if (fp > -1)
                    {
                        while (ftruncate(fp, size) < 0 && errno == EINTR)
                        {
                        }

                        result_mem = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
                        if (result_mem == MAP_FAILED)  result_mem = NULL;
                        else
                        {
                            if (result_mem[0] == '\x00')
                            {
                                while (result_mem[0] == '\x00')
                                {
                                    usleep(2000);
                                }
                            }

                            char *mem_ptr = result_mem + align_unix_size(1);
                            mutex_ptr = reinterpret_cast<pthread_mutex_t *>(mem_ptr);
                            mem_ptr += align_unix_size(sizeof(pthread_mutex_t));

                            ref_count_ptr = reinterpret_cast<uint32_t *>(mem_ptr);
                            mem_ptr += align_unix_size(sizeof(uint32_t));

                            pthread_mutex_lock(mutex_ptr);

                            if (ref_count_ptr[0])  result = 1;
                            else
                            {
                                memset(mem_ptr, 0, size);

                                result = 0;
                            }

                            ref_count_ptr[0]++;

                            pthread_mutex_unlock(mutex_ptr);
                        }

                        close(fp);
                    }
                }

                umask(prev_mask);
            }

            return result;
        }

		static void unix_named_mem_ready(char *mem_ptr)
        {
            pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t *>(mem_ptr + align_unix_size(1)));
        }

		static void unmap_unix_named_mem(char *mem_ptr, size_t size)
        {
            pthread_mutex_t *mutex_ptr;
            uint32_t *ref_count_ptr;

            char *mem_ptr2 = mem_ptr + align_unix_size(1);
            mutex_ptr = reinterpret_cast<pthread_mutex_t *>(mem_ptr2);
            mem_ptr2 += align_unix_size(sizeof(pthread_mutex_t));

            ref_count_ptr = reinterpret_cast<uint32_t *>(mem_ptr2);

            pthread_mutex_lock(mutex_ptr);
            if (ref_count_ptr[0])  ref_count_ptr[0]--;
            pthread_mutex_unlock(mutex_ptr);

            munmap(mem_ptr, align_unix_size(1) + align_unix_size(sizeof(pthread_mutex_t)) + align_unix_size(sizeof(uint32_t)) + size);
        }

		class unix_semaphore_wrapper
		{
		public:
			pthread_mutex_t *_mutex;
			volatile uint32_t *_count;
			volatile uint32_t *_max;
			pthread_cond_t *_condition;
		};

		static size_t get_unix_semaphore_size()
        {
            return align_unix_size(sizeof(pthread_mutex_t)) + align_unix_size(sizeof(uint32_t)) + align_unix_size(sizeof(uint32_t)) + align_unix_size(sizeof(pthread_cond_t));
        }

		static void get_unix_semaphore(unix_semaphore_wrapper& result, char *memory)
        {
            result._mutex = reinterpret_cast<pthread_mutex_t *>(memory);
            memory += align_unix_size(sizeof(pthread_mutex_t));

            result._count = reinterpret_cast<uint32_t *>(memory);
            memory += align_unix_size(sizeof(uint32_t));

            result._max = reinterpret_cast<uint32_t *>(memory);
            memory += align_unix_size(sizeof(uint32_t));

            result._condition = reinterpret_cast<pthread_cond_t *>(memory);
        }

		static void init_unix_semaphore(unix_semaphore_wrapper& unix_semaphore, bool shared, uint32_t start, uint32_t max)
        {
            pthread_mutexattr_t mutex_attr;
            pthread_condattr_t cond_attr;

            pthread_mutexattr_init(&mutex_attr);
            pthread_condattr_init(&cond_attr);

            if (shared)
            {
                pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
                pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
            }

            pthread_mutex_init(unix_semaphore._mutex, &mutex_attr);
            if (start > max)  start = max;
            unix_semaphore._count[0] = start;
            unix_semaphore._max[0] = max;
            pthread_cond_init(unix_semaphore._condition, &cond_attr);

            pthread_condattr_destroy(&cond_attr);
            pthread_mutexattr_destroy(&mutex_attr);
        }

		static bool wait_for_unix_semaphore(unix_semaphore_wrapper& unix_semaphore, uint32_t wait_ms = helper::WaitInfinitely)
        {
            if (wait_ms == 0)
            {
                if (pthread_mutex_trylock(unix_semaphore._mutex) != 0)  return false;
            }
            else
            {
                if (pthread_mutex_lock(unix_semaphore._mutex) != 0)  return false;
            }

            bool result = false;

            if (unix_semaphore._count[0])
            {
                unix_semaphore._count[0]--;

                result = true;
            }
            else if (wait_ms == helper::WaitInfinitely)
            {
                int result2;
                do
                {
                    result2 = pthread_cond_wait(unix_semaphore._condition, unix_semaphore._mutex);
                    if (result2 != 0)  break;
                } while (!unix_semaphore._count[0]);

                if (result2 == 0)
                {
                    unix_semaphore._count[0]--;

                    result = true;
                }
            }
            else if (wait_ms == 0) { /* skip */ }
            else
            {
                struct timespec temp_time;

                if (__csgx__clock_get_time_realtime(&temp_time) == -1)  return false;
                temp_time.tv_sec += wait_ms / 1000;
                temp_time.tv_nsec += (wait_ms % 1000) * 1000000;
                temp_time.tv_sec += temp_time.tv_nsec / 1000000000;
                temp_time.tv_nsec = temp_time.tv_nsec % 1000000000;

                int result2;
                do
                {
                    result2 = pthread_cond_timedwait(unix_semaphore._condition, unix_semaphore._mutex, &temp_time);
                    if (result2 != 0)  break;
                } while (!unix_semaphore._count[0]);

                if (result2 == 0)
                {
                    unix_semaphore._count[0]--;

                    result = true;
                }
            }

            pthread_mutex_unlock(unix_semaphore._mutex);

            return result;
        }

		static bool release_unix_semaphore(unix_semaphore_wrapper& unix_semaphore, uint32_t *prev_val)
        {
            if (pthread_mutex_lock(unix_semaphore._mutex) != 0)  return false;

            if (prev_val != NULL)  *prev_val = unix_semaphore._count[0];
            unix_semaphore._count[0]++;
            if (unix_semaphore._count[0] > unix_semaphore._max[0])  unix_semaphore._count[0] = unix_semaphore._max[0];

            pthread_cond_signal(unix_semaphore._condition);

            pthread_mutex_unlock(unix_semaphore._mutex);

            return true;
        }

		static void free_unix_semaphore(unix_semaphore_wrapper& unix_semaphore)
        {
            pthread_mutex_destroy(unix_semaphore._mutex);
            pthread_cond_destroy(unix_semaphore._condition);
        }

		class unix_event_wrapper
		{
		public:
			pthread_mutex_t *_mutex;
			volatile char *_manual;
			volatile char *_signaled;
			volatile uint32_t *_waiting;
			pthread_cond_t *_condition;
		};

		static size_t get_unix_event_size()
        {
            return align_unix_size(sizeof(pthread_mutex_t)) + align_unix_size(2) + align_unix_size(sizeof(uint32_t)) + align_unix_size(sizeof(pthread_cond_t));
        }

		static void get_unix_event(unix_event_wrapper& result, char *memory)
        {
            result._mutex = reinterpret_cast<pthread_mutex_t *>(memory);
            memory += align_unix_size(sizeof(pthread_mutex_t));

            result._manual = memory;
            result._signaled = memory + 1;
            memory += align_unix_size(2);

            result._waiting = reinterpret_cast<uint32_t *>(memory);
            memory += align_unix_size(sizeof(uint32_t));

            result._condition = reinterpret_cast<pthread_cond_t *>(memory);
        }

		static void init_unix_event(unix_event_wrapper& unix_event, bool shared, bool manual, bool signaled = false)
        {
            pthread_mutexattr_t mutex_attr;
            pthread_condattr_t cond_attr;

            pthread_mutexattr_init(&mutex_attr);
            pthread_condattr_init(&cond_attr);

            if (shared)
            {
                pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
                pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
            }

            pthread_mutex_init(unix_event._mutex, &mutex_attr);
            unix_event._manual[0] = (manual ? '\x01' : '\x00');
            unix_event._signaled[0] = (signaled ? '\x01' : '\x00');
            unix_event._waiting[0] = 0;
            pthread_cond_init(unix_event._condition, &cond_attr);

            pthread_condattr_destroy(&cond_attr);
            pthread_mutexattr_destroy(&mutex_attr);
        }

		static bool wait_for_unix_event(unix_event_wrapper& unix_event, uint32_t wait_ms = WaitInfinitely)
        {
            if (wait_ms == 0)
            {
                if (pthread_mutex_trylock(unix_event._mutex) != 0)  return false;
            }
            else
            {
                if (pthread_mutex_lock(unix_event._mutex) != 0)  return false;
            }

            bool result = false;

            if (unix_event._signaled[0] != '\x00' && (unix_event._manual[0] != '\x00' || !unix_event._waiting[0]))
            {
                if (unix_event._manual[0] == '\x00')  unix_event._signaled[0] = '\x00';

                result = true;
            }
            else if (wait_ms == helper::WaitInfinitely)
            {
                unix_event._waiting[0]++;

                int result2;
                do
                {
                    result2 = pthread_cond_wait(unix_event._condition, unix_event._mutex);
                    if (result2 != 0)  break;
                } while (unix_event._signaled[0] == '\x00');

                unix_event._waiting[0]--;

                if (result2 == 0)
                {
                    if (unix_event._manual[0] == '\x00')  unix_event._signaled[0] = '\x00';

                    result = true;
                }
            }
            else if (wait_ms == 0) { /* skip */ }
            else
            {
                struct timespec temp_time;

                if (__csgx__clock_get_time_realtime(&temp_time) == -1)
                {
                    pthread_mutex_unlock(unix_event._mutex);

                    return false;
                }

                temp_time.tv_sec += wait_ms / 1000;
                temp_time.tv_nsec += (wait_ms % 1000) * 1000000;
                temp_time.tv_sec += temp_time.tv_nsec / 1000000000;
                temp_time.tv_nsec = temp_time.tv_nsec % 1000000000;

                unix_event._waiting[0]++;

                int result2;
                do
                {
                    result2 = pthread_cond_timedwait(unix_event._condition, unix_event._mutex, &temp_time);
                    if (result2 != 0)  break;
                } while (unix_event._signaled[0] == '\x00');

                unix_event._waiting[0]--;

                if (result2 == 0)
                {
                    if (unix_event._manual[0] == '\x00')  unix_event._signaled[0] = '\x00';

                    result = true;
                }
            }

            pthread_mutex_unlock(unix_event._mutex);

            return result;
        }

		static bool fire_unix_event(unix_event_wrapper& unix_event)
        {
            if (pthread_mutex_lock(unix_event._mutex) != 0)  return false;

            unix_event._signaled[0] = '\x01';

            if (unix_event._manual[0] != '\x00')  pthread_cond_broadcast(unix_event._condition);
            else  pthread_cond_signal(unix_event._condition);

            pthread_mutex_unlock(unix_event._mutex);

            return true;
        }

		static bool reset_unix_event(unix_event_wrapper& unix_event)
        {
            if (unix_event._manual[0] == '\x00')  return false;
            if (pthread_mutex_lock(unix_event._mutex) != 0)  return false;

            unix_event._signaled[0] = '\x00';

            pthread_mutex_unlock(unix_event._mutex);

            return true;
        }

		static void free_unix_event(unix_event_wrapper& unix_event)
        {
            pthread_mutex_destroy(unix_event._mutex);
            pthread_cond_destroy(unix_event._condition);
        }
#endif
	};

	class global_named_mutex
	{
	public:
		global_named_mutex()
        {
#ifdef WINDOWS_OS
            ::InitializeCriticalSection(&_critical_section);
#else
            pthread_mutex_init(&_critical_section, NULL);
#endif
        }

		~global_named_mutex()
        {
#ifdef WINDOWS_OS
            unlock(true);

            if (_mutex != nullptr)  ::CloseHandle(_mutex);
            ::DeleteCriticalSection(&_critical_section);
#else
            unlock(true);

            if (_mutex_memory != nullptr)
            {
                if (_named)  helper::unmap_unix_named_mem(_mutex_memory, helper::get_unix_semaphore_size());
                else
                {
                    helper::free_unix_semaphore(_thread_mutex);

                    delete[] _mutex_memory;
                }
            }

            pthread_mutex_destroy(&_critical_section);
#endif
        }

		bool create(const char* name = nullptr)
        {
#ifdef WINDOWS_OS
            ::EnterCriticalSection(&_critical_section);

            if (_count > 0)
            {
                if (_owner_id == helper::get_current_thread_id())
                {
                    ::ReleaseMutex(_mutex);
                    ::CloseHandle(_mutex);
                    _mutex = nullptr;
                    _count = 0;
                    _owner_id = 0;
                }
                else
                {
                    ::LeaveCriticalSection(&_critical_section);

                    return false;
                }
            }

            SECURITY_ATTRIBUTES sec_attr;

            sec_attr.nLength = sizeof(sec_attr);
            sec_attr.lpSecurityDescriptor = nullptr;
            sec_attr.bInheritHandle = TRUE;

            _mutex = ::CreateMutexA(&sec_attr, FALSE, name);

            if (_mutex == nullptr)
            {
                ::LeaveCriticalSection(&_critical_section);

                return false;
            }

            ::LeaveCriticalSection(&_critical_section);

            return true;
#else
            if (pthread_mutex_lock(&_critical_section) != 0)  return false;

            if (_count > 0)
            {
                if (_owner_id == helper::get_current_thread_id())
                {
                    if (_named)  helper::unmap_unix_named_mem(_mutex_memory, helper::get_unix_semaphore_size());
                    else
                    {
                        helper::free_unix_semaphore(_thread_mutex);

                        delete[] _mutex_memory;
                    }

                    _named = false;
                    _mutex_memory = nullptr;
                    _count = 0;
                    _owner_id = 0;
                }
                else
                {
                    pthread_mutex_unlock(&_critical_section);

                    return false;
                }
            }

            size_t pos, temp_size = helper::get_unix_semaphore_size();
            _named = name != nullptr;
            int result = helper::init_unix_named_mem(_mutex_memory, pos, "/Sync_Mutex", name, temp_size);

            if (result < 0)
            {
                pthread_mutex_unlock(&_critical_section);

                return false;
            }

            helper::get_unix_semaphore(_thread_mutex, _mutex_memory + pos);

            if (result == 0)
            {
                helper::init_unix_semaphore(_thread_mutex, _named, 1, 1);

                if (_named)  helper::unix_named_mem_ready(_mutex_memory);
            }

            pthread_mutex_unlock(&_critical_section);

            return true;
#endif
        }

		bool lock(uint32_t wait_ms = helper::WaitInfinitely)
        {
#ifdef WINDOWS_OS
            ::EnterCriticalSection(&_critical_section);

            if (_mutex == nullptr)
            {
                ::LeaveCriticalSection(&_critical_section);

                return false;
            }

            if (_owner_id == helper::get_current_thread_id())
            {
                _count++;
                ::LeaveCriticalSection(&_critical_section);

                return true;
            }

            ::LeaveCriticalSection(&_critical_section);

            DWORD result = ::WaitForSingleObject(_mutex, (DWORD)wait_ms);
            if (result != WAIT_OBJECT_0)  return false;

            ::EnterCriticalSection(&_critical_section);
            _owner_id = helper::get_current_thread_id();
            _count = 1;
            ::LeaveCriticalSection(&_critical_section);

            return true;
#else
            if (pthread_mutex_lock(&_critical_section) != 0)  return false;

            if (_owner_id == helper::get_current_thread_id())
            {
                _count++;
                pthread_mutex_unlock(&_critical_section);

                return true;
            }

            pthread_mutex_unlock(&_critical_section);

            if (!helper::wait_for_unix_semaphore(_thread_mutex, wait_ms))  return false;

            pthread_mutex_lock(&_critical_section);
            _owner_id = helper::get_current_thread_id();
            _count = 1;
            pthread_mutex_unlock(&_critical_section);

            return true;
#endif
        }

		bool unlock(bool all = false)
        {
#ifdef WINDOWS_OS
            ::EnterCriticalSection(&_critical_section);

            if (_mutex == nullptr || _owner_id != helper::get_current_thread_id())
            {
                ::LeaveCriticalSection(&_critical_section);

                return false;
            }

            if (all)  _count = 1;
            _count--;
            if (!_count)
            {
                _owner_id = 0;

                ::ReleaseMutex(_mutex);
            }

            ::LeaveCriticalSection(&_critical_section);

            return true;
#else
        if (pthread_mutex_lock(&_critical_section) != 0)  return false;

            if (_owner_id != helper::get_current_thread_id())
            {
                pthread_mutex_unlock(&_critical_section);

                return false;
            }

            if (all)  _count = 1;
            _count--;
            if (!_count)
            {
                _owner_id = 0;

                // Release the mutex.
                helper::release_unix_semaphore(_thread_mutex, NULL);
            }

            pthread_mutex_unlock(&_critical_section);

            return true;
#endif
        }

		class auto_unlock
		{
		public:
			auto_unlock(global_named_mutex* lock_ptr)
            {
                _lock = lock_ptr;
            }

			~auto_unlock()
            {
                _lock->unlock();
            }

		private:
			auto_unlock(const auto_unlock &) = delete;
			auto_unlock &operator=(const auto_unlock &) = delete;

			global_named_mutex *_lock;
		};

	private:
		global_named_mutex(const global_named_mutex &) = delete;
		global_named_mutex &operator=(const global_named_mutex &) = delete;

#ifdef WINDOWS_OS
		CRITICAL_SECTION _critical_section;
		HANDLE _mutex { nullptr };
#else
		char* _mutex_memory { nullptr };
		bool _named { false };
		pthread_mutex_t _critical_section;
		helper::unix_semaphore_wrapper _thread_mutex { 0 };
#endif

		volatile thread_id_type _owner_id { 0 };
		volatile uint32_t _count { 0 };
	};
}

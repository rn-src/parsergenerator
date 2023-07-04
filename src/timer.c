#ifdef _MSC_VER
#include <windows.h>
#include <winnt.h>
#else
#include <unistd.h>
#include <sys/times.h>
#endif
#include <stdint.h>

typedef uint64_t ticks_t;

ticks_t getSystemTicks()
{
#ifdef WIN32
  LARGE_INTEGER timer;
  QueryPerformanceCounter(&timer);
  return timer.QuadPart;
#else
  struct tms now;
  clock_t ticks = times(&now);
  return ticks;
#endif
}

ticks_t getSystemTicksFreq()
{
#ifdef WIN32
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
#else
  return sysconf(_SC_CLK_TCK);
#endif
}

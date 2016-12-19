/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TestUtils_Safety_H
#define TestUtils_Safety_H

#if defined(OPENDDS_SAFETY_PROFILE) && (defined(__GLIBC__) || defined(ACE_HAS_EXECINFO_H))

#include <cstdlib>

static bool no_global_new = false;

class DisableGlobalNew {
public:
  DisableGlobalNew() {
#ifdef ACE_FACE_SAFETY_BASE
    no_global_new = true;
#else
    no_global_new = false; // Safety extended allows new
#endif
  }
};

#include <execinfo.h>

void* operator new(size_t sz)
#ifdef ACE_HAS_NEW_THROW_SPEC
  throw (std::bad_alloc)
#endif
 {
  if (no_global_new) {
    printf ("ERROR: call to global operator new\n");
  }
  return ::malloc(sz);
}

void operator delete(void* ptr) {
  ::free(ptr);
}

void* operator new(size_t sz, const std::nothrow_t&)
{
  if (no_global_new) {
    printf ("ERROR: call to global operator new\n");
  }
  return ::malloc(sz);
}

void operator delete(void* ptr, const std::nothrow_t&) {
  ::free(ptr);
}

void* operator new[](size_t sz, const std::nothrow_t&)
{
  if (no_global_new) {
    printf ("ERROR: call to global operator new[]\n");
    void* addresses[32];
    int count = backtrace(addresses, 32);
    char** text = backtrace_symbols(addresses, count);
    for (int i = 0; i != count; ++i) {
      printf ("%s\n", text[i]);
    }
    ::free(text);
  }
  return ::malloc(sz);
}

void operator delete[](void* ptr, const std::nothrow_t&) {
  ::free(ptr);
}

void* operator new[](size_t sz)
#ifdef ACE_HAS_NEW_THROW_SPEC
  throw (std::bad_alloc)
#endif
{
  if (no_global_new) {
    printf ("ERROR: call to global operator new[]\n");
    void* addresses[32];
    int count = backtrace(addresses, 32);
    char** text = backtrace_symbols(addresses, count);
    for (int i = 0; i != count; ++i) {
      printf ("%s\n", text[i]);
    }
    ::free(text);
  }
  return ::malloc(sz);
}

void operator delete[](void* ptr) {
  ::free(ptr);
}

#else

class DisableGlobalNew {
public:
  DisableGlobalNew() { }
};

#endif

#endif /* TestUtils_Safety_H */

// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopyAllocator_T.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef ZEROCOPYALLOCATOR_T_CPP
#define ZEROCOPYALLOCATOR_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/ZeroCopyAllocator_T.h"

#if !defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopyAllocator_T.inl"
#endif /* __//ACE_INLINE__ */

namespace OpenDDS
{
    namespace DCPS
    {


//=============================================================
// These methods are no-ops.
//=============================================================
template <class T, std::size_t N>
void *
FirstTimeFastAllocator<T, N>::calloc (size_t /* nbytes */,
				      char /* initial_value */)
{
  /* no-op */
  return (void*)0;
}

template <class T, std::size_t N>
void *
FirstTimeFastAllocator<T, N>::calloc (size_t /* n_elem */,
				      size_t /* elem_size */,
				      char /* initial_value */)
{
  /* no-op */
  return (void*)0;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::remove (void)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::bind (const char * /* name */,
				    void * /* pointer */,
				    int /* duplicates */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::trybind (const char * /* name */,
				       void *& /* pointer */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::find (const char * /* name */,
				    void *& /* pointer */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::find (const char * /* name */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::unbind (const char * /* name */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::unbind (const char * /* name */,
				      void *& /* pointer */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::sync (ssize_t /* len */,
				    int /* flags */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::sync (void * /* addr */,
				    size_t /* len */,
				    int /* flags */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::protect (ssize_t /* len */,
				       int /* prot */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::protect (void * /* addr */,
				       size_t /* len */,
				       int /* prot */)
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
  return -1;
}

#if defined (ACE_HAS_MALLOC_STATS)

template <class T, std::size_t N>
void
FirstTimeFastAllocator<T, N>::print_stats (void) const
{
  /* no-op */
  ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
}
#endif /* ACE_HAS_MALLOC_STATS */

template <class T, std::size_t N>
void
FirstTimeFastAllocator<T, N>::dump (void) const
{/* no-op */
    ACE_ASSERT("not supported" == reinterpret_cast<char*>(0));
}

    } // namespace  ::DDS
} // namespace OpenDDS


#endif /* ZEROCOPYALLOCATOR_T_CPP  */




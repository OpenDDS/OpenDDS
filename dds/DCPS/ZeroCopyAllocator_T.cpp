/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYALLOCATOR_T_CPP
#define ZEROCOPYALLOCATOR_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/ZeroCopyAllocator_T.h"

#if !defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopyAllocator_T.inl"
#endif /* __//ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

//=============================================================
// These methods are no-ops.
//=============================================================
template <class T, std::size_t N>
void *
FirstTimeFastAllocator<T, N>::calloc(size_t nbytes, char initial_value)
{/* no-op */
  ACE_UNUSED_ARG(nbytes);
  ACE_UNUSED_ARG(initial_value);
  return (void*)0;
}

template <class T, std::size_t N>
void *
FirstTimeFastAllocator<T, N>::calloc(size_t n_elem, size_t elem_size, char initial_value)
{/* no-op */
  ACE_UNUSED_ARG(n_elem);
  ACE_UNUSED_ARG(elem_size);
  ACE_UNUSED_ARG(initial_value);
  return (void*)0;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::remove()
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::bind(const char *name, void *pointer, int duplicates)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(name);
  ACE_UNUSED_ARG(pointer);
  ACE_UNUSED_ARG(duplicates);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::trybind(const char *name, void *&pointer)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(name);
  ACE_UNUSED_ARG(pointer);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::find(const char *name, void *&pointer)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(name);
  ACE_UNUSED_ARG(pointer);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::find(const char *name)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(name);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::unbind(const char *name)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(name);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::unbind(const char *name, void *&pointer)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(name);
  ACE_UNUSED_ARG(pointer);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::sync(ssize_t len, int flags)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(len);
  ACE_UNUSED_ARG(flags);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::sync(void *addr, size_t len, int flags)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(addr);
  ACE_UNUSED_ARG(len);
  ACE_UNUSED_ARG(flags);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::protect(ssize_t len, int prot)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(len);
  ACE_UNUSED_ARG(prot);
  return -1;
}

template <class T, std::size_t N>
int
FirstTimeFastAllocator<T, N>::protect(void *addr, size_t len, int prot)
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
  ACE_UNUSED_ARG(addr);
  ACE_UNUSED_ARG(len);
  ACE_UNUSED_ARG(prot);
  return -1;
}

#if defined (ACE_HAS_MALLOC_STATS)

template <class T, std::size_t N>
void
FirstTimeFastAllocator<T, N>::print_stats() const
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
}
#endif /* ACE_HAS_MALLOC_STATS */

template <class T, std::size_t N>
void
FirstTimeFastAllocator<T, N>::dump() const
{/* no-op */
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) %T not supported %a\n"))) ;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* ZEROCOPYALLOCATOR_T_CPP  */

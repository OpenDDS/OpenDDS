// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopyInfoSeq_T.inl
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "ZeroCopyInfoSeq_T.h"

namespace TAO
{
    namespace DCPS
    {


template <class InfoType, size_t DEF_MAX> ACE_INLINE
ZeroCopyInfoSeq<InfoType, DEF_MAX>::ZeroCopyInfoSeq()
  : TAO_BASE_SEQUENCE <InfoType> ()
{
}


template <class InfoType, size_t DEF_MAX> ACE_INLINE
ZeroCopyInfoSeq<InfoType, DEF_MAX>::ZeroCopyInfoSeq(
  CORBA::ULong maximum,
  CORBA::ULong init_size,
  ACE_Allocator*)
  : TAO_BASE_SEQUENCE <InfoType> (maximum ? maximum : init_size)
{
}


template <class InfoType, size_t DEF_MAX> ACE_INLINE
ZeroCopyInfoSeq<InfoType, DEF_MAX>::ZeroCopyInfoSeq(
  CORBA::ULong maximum,
  CORBA::ULong length,
  InfoType* buffer,
  CORBA::Boolean release)
  : TAO_BASE_SEQUENCE <InfoType> (maximum, length, buffer, release)
{
}


    } // namespace  ::DDS
} // namespace OpenDDS

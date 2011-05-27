/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ZeroCopyInfoSeq_T.h"

namespace TAO {
namespace DCPS {

template <class InfoType, size_t DEF_MAX> ACE_INLINE
ZeroCopyInfoSeq<InfoType, DEF_MAX>::ZeroCopyInfoSeq()
  : TAO::unbounded_value_sequence<InfoType>()
{
}

template <class InfoType, size_t DEF_MAX> ACE_INLINE
ZeroCopyInfoSeq<InfoType, DEF_MAX>::ZeroCopyInfoSeq(
  CORBA::ULong maximum,
  CORBA::ULong init_size,
  ACE_Allocator*)
    : TAO::unbounded_value_sequence<InfoType>(maximum ? maximum : init_size)
{
}

template <class InfoType, size_t DEF_MAX> ACE_INLINE
ZeroCopyInfoSeq<InfoType, DEF_MAX>::ZeroCopyInfoSeq(
  CORBA::ULong maximum,
  CORBA::ULong length,
  InfoType* buffer,
  CORBA::Boolean release)
    : TAO::unbounded_value_sequence<InfoType>(maximum, length, buffer, release)
{
}

} // namespace DCPS
} // namespace TAO

/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYINFOSEQ_H
#define ZEROCOPYINFOSEQ_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include /**/ "ace/pre.h"

#include "dds/DCPS/ZeroCopySeqBase.h"

// kludge to be sure LocalObject is defined in DdsDcpsInfrastructureC.h
// because a listener might be defined without #including
// any non-listener entity implementations.
#include "dds/DCPS/LocalObject.h"

#include <tao/Unbounded_Value_Sequence_T.h>

//This must stay in namespace "TAO" until the tao_idl compiler is changed
namespace TAO {
namespace DCPS {

template <class InfoType, size_t DEF_MAX = DCPS_ZERO_COPY_SEQ_DEFAULT_SIZE>
class ZeroCopyInfoSeq : public TAO::unbounded_value_sequence<InfoType> {
public:

  ZeroCopyInfoSeq();

  //This ctor serves as the CORBA "max" ctor as well as providing
  //symmetry with the ZeroCopyDataSeq class template.
  explicit ZeroCopyInfoSeq(CORBA::ULong maximum,
                           CORBA::ULong init_size = DEF_MAX, ACE_Allocator* = 0);

  ZeroCopyInfoSeq(CORBA::ULong maximum, CORBA::ULong length,
                  InfoType* buffer, CORBA::Boolean release = false);

};

} // namespace DCPS
} // namespace TAO

#if defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopyInfoSeq_T.inl"
#endif /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "dds/DCPS/ZeroCopyInfoSeq_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("ZeroCopyInfoSeq_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#include /**/ "ace/post.h"

#endif /* ZEROCOPYINFOSEQ_H  */

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsCustomizedElement.h"
#include "RtpsSampleHeader.h"

#ifndef __ACE_INLINE__
#include "RtpsCustomizedElement.inl"
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsCustomizedElement::~RtpsCustomizedElement()
{}

ElementPair
RtpsCustomizedElement::fragment(size_t size)
{
  ACE_Message_Block* head;
  ACE_Message_Block* tail;
  const SequenceRange fragNumbers =
    RtpsSampleHeader::split(*msg(), size, head, tail);

  RtpsCustomizedElement* frag =
    RtpsCustomizedElement::alloc(0, head, allocator());
  frag->set_publication_id(publication_id());
  frag->seq_ = sequence();
  frag->set_fragment();
  frag->last_frag_ = fragNumbers.first;

  RtpsCustomizedElement* rest =
    RtpsCustomizedElement::alloc(this, tail, allocator());
  rest->set_fragment();
  rest->last_frag_ = fragNumbers.second;

  return ElementPair(frag, rest);
}

const ACE_Message_Block*
RtpsCustomizedElement::msg_payload() const
{
  return msg() ? msg()->cont() : 0;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

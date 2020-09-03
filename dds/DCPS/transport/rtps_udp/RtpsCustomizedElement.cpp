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

TqePair RtpsCustomizedElement::fragment(size_t size)
{
  Message_Block_Ptr head;
  Message_Block_Ptr tail;
  const SequenceRange fragNumbers =
    RtpsSampleHeader::split(*msg(), size, head, tail);
  if (fragNumbers == unknown_sequence_range) {
    return null_tqe_pair;
  }

  RtpsCustomizedElement* frag = new RtpsCustomizedElement(0, move(head));
  frag->set_fragment(this);
  frag->last_frag_ = fragNumbers.first;

  RtpsCustomizedElement* rest = new RtpsCustomizedElement(this, move(tail));
  rest->set_fragment(this);
  rest->last_frag_ = fragNumbers.second;

  return TqePair(frag, rest);
}

const ACE_Message_Block*
RtpsCustomizedElement::msg_payload() const
{
  const ACE_Message_Block* message = msg();
  return message ? message->cont() : 0;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

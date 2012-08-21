/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

RtpsCustomizedElement::~RtpsCustomizedElement()
{}

ElementPair
RtpsCustomizedElement::fragment(size_t size)
{
  ACE_Message_Block* head;
  ACE_Message_Block* tail;
  RtpsSampleHeader::split(*msg(), size, head, tail);

  RtpsCustomizedElement* frag =
    RtpsCustomizedElement::alloc(0, head, allocator());
  frag->set_publication_id(publication_id());
  frag->set_fragment();

  RtpsCustomizedElement* rest =
    RtpsCustomizedElement::alloc(this, tail, allocator());
  rest->set_fragment();

  return ElementPair(frag, rest);
}

}
}

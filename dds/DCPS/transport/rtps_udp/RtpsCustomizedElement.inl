/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
RtpsCustomizedElement::RtpsCustomizedElement(TransportQueueElement* orig,
                                             ACE_Message_Block* msg,
                                             ACE_Allocator* allocator)
  : TransportCustomizedElement(orig, false, allocator)
  , seq_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
{
  set_requires_exclusive();
  set_msg(msg);
}

ACE_INLINE /*static*/
RtpsCustomizedElement*
RtpsCustomizedElement::alloc(TransportQueueElement* orig,
                             ACE_Message_Block* msg,
                             ACE_Allocator* allocator /* = 0 */)
{
  if (allocator) {
    RtpsCustomizedElement* ret;
    ACE_NEW_MALLOC_RETURN(ret,
      static_cast<RtpsCustomizedElement*>(allocator->malloc(0)),
      RtpsCustomizedElement(orig, msg, allocator),
      0);
    return ret;
  } else {
    return new RtpsCustomizedElement(orig, msg, 0);
  }
}

ACE_INLINE
SequenceNumber
RtpsCustomizedElement::sequence() const
{
  return (seq_ == SequenceNumber::SEQUENCENUMBER_UNKNOWN()) ?
    TransportCustomizedElement::sequence() : seq_;
}

ACE_INLINE
SequenceNumber
RtpsCustomizedElement::last_fragment() const
{
  return last_frag_;
}

}
}

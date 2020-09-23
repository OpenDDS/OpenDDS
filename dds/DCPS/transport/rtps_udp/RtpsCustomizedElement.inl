/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
RtpsCustomizedElement::RtpsCustomizedElement(TransportQueueElement* orig,
                                             Message_Block_Ptr msg)
  : TransportCustomizedElement(orig)
{
  set_requires_exclusive();
  set_msg(move(msg));
}

ACE_INLINE
SequenceNumber
RtpsCustomizedElement::last_fragment() const
{
  return last_frag_;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

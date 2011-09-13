/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

//TODO: implement

ACE_INLINE size_t
RtpsTransportHeader::max_marshaled_size()
{
  return 0;
}

ACE_INLINE
RtpsTransportHeader::RtpsTransportHeader()
{
}

ACE_INLINE
RtpsTransportHeader::RtpsTransportHeader(ACE_Message_Block& mb)
{
}

ACE_INLINE RtpsTransportHeader&
RtpsTransportHeader::operator=(ACE_Message_Block& mb)
{
  return *this;
}

ACE_INLINE bool
RtpsTransportHeader::valid() const
{
  return true;
}

ACE_INLINE bool
RtpsTransportHeader::first_fragment()
{
  return false;
}

ACE_INLINE bool
RtpsTransportHeader::last_fragment()
{
  return false;
}

ACE_INLINE void
RtpsTransportHeader::last_fragment(bool frag)
{
}

ACE_INLINE const SequenceNumber&
RtpsTransportHeader::sequence()
{
  return seq_;
}

}
}

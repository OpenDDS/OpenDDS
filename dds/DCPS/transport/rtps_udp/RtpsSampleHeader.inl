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
RtpsSampleHeader::max_marshaled_size()
{
  return 0;
}

ACE_INLINE bool
RtpsSampleHeader::partial(const ACE_Message_Block& mb)
{
  return false;
}

ACE_INLINE
RtpsSampleHeader::RtpsSampleHeader()
{
}

ACE_INLINE
RtpsSampleHeader::RtpsSampleHeader(ACE_Message_Block& mb)
{
}

ACE_INLINE RtpsSampleHeader&
RtpsSampleHeader::operator=(ACE_Message_Block& mb)
{
  return *this;
}

ACE_INLINE bool
RtpsSampleHeader::valid() const
{
  return true;
}

ACE_INLINE size_t
RtpsSampleHeader::marshaled_size()
{
  return 0;
}

ACE_INLINE void
RtpsSampleHeader::into_received_data_sample(ReceivedDataSample& rds)
{
}

ACE_INLINE ACE_UINT32
RtpsSampleHeader::message_length()
{
  return 0;
}

ACE_INLINE bool
RtpsSampleHeader::more_fragments() const
{
  return false;
}

}
}

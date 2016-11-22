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
RtpsSampleHeader::RtpsSampleHeader()
  : valid_(false)
  , frag_(false)
  , marshaled_size_(0)
  , message_length_(0)
{
}

ACE_INLINE
RtpsSampleHeader::RtpsSampleHeader(ACE_Message_Block& mb)
  : valid_(false)
  , frag_(false)
  , marshaled_size_(0)
  , message_length_(0)
{
  init(mb);
}

ACE_INLINE RtpsSampleHeader&
RtpsSampleHeader::operator=(ACE_Message_Block& mb)
{
  frag_ = false;
  valid_ = false;
  // message_length_ should not be reset here
  // marshaled_size_ doesn't need to be reset, init() will set it (if valid_)
  init(mb);
  return *this;
}

ACE_INLINE bool
RtpsSampleHeader::valid() const
{
  return valid_;
}

ACE_INLINE void
RtpsSampleHeader::pdu_remaining(size_t size)
{
  message_length_ = size;
}

ACE_INLINE size_t
RtpsSampleHeader::marshaled_size()
{
  return marshaled_size_;
}

ACE_INLINE ACE_UINT32
RtpsSampleHeader::message_length()
{
  return static_cast<ACE_UINT32>(message_length_);
}

ACE_INLINE bool
RtpsSampleHeader::more_fragments() const
{
  return frag_;
}

ACE_INLINE bool
RtpsSampleHeader::control_message_supported(char message_id)
{
  switch (message_id) {
  case INSTANCE_REGISTRATION:
  case UNREGISTER_INSTANCE:
  case DISPOSE_INSTANCE:
  case DISPOSE_UNREGISTER_INSTANCE:
    return true;
  default:
    return false;
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

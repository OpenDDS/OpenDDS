/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsTransportHeader.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"

#ifndef __ACE_INLINE__
#include "RtpsTransportHeader.inl"
#endif

namespace {
  const ACE_CDR::Octet PROTOCOL_RTPS[] = {'R', 'T', 'P', 'S'};
  const OpenDDS::DCPS::SequenceNumber dummy;
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void
RtpsTransportHeader::init(ACE_Message_Block& mb)
{
  // Byte order doesn't matter for RTPS::Header, since it's
  // exclusively structs/arrays of octet.
  Serializer ser(&mb, false, Serializer::ALIGN_CDR);
  valid_ = (ser >> header_);

  if (valid_) {

    // length_ started as the total number of bytes in the datagram's payload.
    // When we return to the TransportReceiveStrategy it must be the number
    // of bytes remaining after processing this RTPS::Header.
    length_ -= max_marshaled_size();

    // RTPS spec v2.1 section 8.3.6.3
    valid_ = std::equal(header_.prefix, header_.prefix + sizeof(header_.prefix),
                        PROTOCOL_RTPS);
    valid_ &= (header_.version.major == OpenDDS::RTPS::PROTOCOLVERSION.major);
  }
}

const SequenceNumber&
RtpsTransportHeader::sequence()
{
  return dummy;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

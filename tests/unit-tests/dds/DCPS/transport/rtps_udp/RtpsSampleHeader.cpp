/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/RtpsSampleHeader.h>

#include <dds/DCPS/Message_Block_Ptr.h>

using namespace OpenDDS::DCPS;

namespace {
  template <std::size_t N>
  ACE_Message_Block* array_to_mb(const unsigned char (&array)[N])
  {
    ACE_Message_Block* mb = new ACE_Message_Block(reinterpret_cast<const char*>(array), N);
    mb->wr_ptr(N);
    return mb;
  }
}

TEST(dds_DCPS_transport_rtps_udp_RtpsSampleHeader, DataFragValid)
{
  static const unsigned char x[] = {
    0x16,0x01,0x00,0x00, // Submessage Header: DATA_FRAG, FLAG_E, 0 octets to next hdr
    0x00,0x00,0x1c,0x00, // extraFlags, octetsToInlineQoS
    0x00,0x00,0x00,0x00, // readerId
    0x00,0x01,0x02,0x03, // writerId
    0x00,0x00,0x00,0x00, // seq#-high
    0x01,0x00,0x00,0x00, // seq#-low
    0x01,0x00,0x00,0x00, // frag#-start
    0x01,0x00,0x00,0x04, // numFrags, fragSize
    0x99,0x00,0x00,0x00, // sampleSize
  };
  Message_Block_Ptr mb(array_to_mb(x));
  RtpsSampleHeader rsh;
  rsh.pdu_remaining(0x99 + sizeof x);
  rsh = *mb;
  ASSERT_TRUE(rsh.valid());
}

TEST(dds_DCPS_transport_rtps_udp_RtpsSampleHeader, DataFragInvalidSize)
{
  static const unsigned char x[] = {
    0x16,0x01,0x00,0x00, // Submessage Header: DATA_FRAG, FLAG_E, 0 octets to next hdr
    0x00,0x00,0x1c,0x00, // extraFlags, octetsToInlineQoS
    0x00,0x00,0x00,0x00, // readerId
    0x00,0x01,0x02,0x03, // writerId
    0x00,0x00,0x00,0x00, // seq#-high
    0x01,0x00,0x00,0x00, // seq#-low
    0x01,0x00,0x00,0x00, // frag#-start
    0x01,0x00,0x00,0x00, // numFrags, fragSize
    0x99,0x00,0x00,0x00, // sampleSize
  };
  Message_Block_Ptr mb(array_to_mb(x));
  RtpsSampleHeader rsh;
  rsh.pdu_remaining(0x99 + sizeof x);
  rsh = *mb;
  ASSERT_FALSE(rsh.valid());
}

TEST(dds_DCPS_transport_rtps_udp_RtpsSampleHeader, DataFragInvalidStart)
{
  static const unsigned char x[] = {
    0x16,0x01,0x00,0x00, // Submessage Header: DATA_FRAG, FLAG_E, 0 octets to next hdr
    0x00,0x00,0x1c,0x00, // extraFlags, octetsToInlineQoS
    0x00,0x00,0x00,0x00, // readerId
    0x00,0x01,0x02,0x03, // writerId
    0x00,0x00,0x00,0x00, // seq#-high
    0x01,0x00,0x00,0x00, // seq#-low
    0x00,0x00,0x00,0x00, // frag#-start
    0x01,0x00,0x00,0x04, // numFrags, fragSize
    0x99,0x00,0x00,0x00, // sampleSize
  };
  Message_Block_Ptr mb(array_to_mb(x));
  RtpsSampleHeader rsh;
  rsh.pdu_remaining(0x99 + sizeof x);
  rsh = *mb;
  ASSERT_FALSE(rsh.valid());
}

TEST(dds_DCPS_transport_rtps_udp_RtpsSampleHeader, DataFragInvalidEnd)
{
  static const unsigned char x[] = {
    0x16,0x01,0x00,0x00, // Submessage Header: DATA_FRAG, FLAG_E, 0 octets to next hdr
    0x00,0x00,0x1c,0x00, // extraFlags, octetsToInlineQoS
    0x00,0x00,0x00,0x00, // readerId
    0x00,0x01,0x02,0x03, // writerId
    0x00,0x00,0x00,0x00, // seq#-high
    0x01,0x00,0x00,0x00, // seq#-low
    0x00,0x00,0x00,0x00, // frag#-start
    0x02,0x00,0x00,0x04, // numFrags, fragSize
    0x99,0x00,0x00,0x00, // sampleSize
  };
  Message_Block_Ptr mb(array_to_mb(x));
  RtpsSampleHeader rsh;
  rsh.pdu_remaining(0x99 + sizeof x);
  rsh = *mb;
  ASSERT_FALSE(rsh.valid());
}


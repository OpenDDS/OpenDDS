#ifndef TEST_DDS_DCPS_TRANSPORT_RTPS_UDP_UTIL_H
#define TEST_DDS_DCPS_TRANSPORT_RTPS_UDP_UTIL_H

#include <dds/DCPS/RTPS/MessageUtils.h>
#include <dds/DCPS/RTPS/MessageTypes.h>

namespace test {

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

inline bool meta_submessage_equal(const MetaSubmessage& x, const MetaSubmessage& y)
{
  static const Encoding encoding;

  if (x.src_guid_ == y.src_guid_ &&
      x.dst_guid_ == y.dst_guid_ &&
      x.ignore_ == y.ignore_) {
    // FUTURE: Replace this with generated ==.
    size_t x_size = 0;
    serialized_size(encoding, x_size, x.sm_);
    size_t y_size = 0;
    serialized_size(encoding, y_size, y.sm_);
    if (x_size != y_size) {
      return false;
    }

    ACE_Message_Block x_block(x_size);
    Serializer x_ser(&x_block, encoding);
    if (!(x_ser << x.sm_)) {
      return false;
    }

    ACE_Message_Block y_block(y_size);
    Serializer y_ser(&y_block, encoding);
    if (!(y_ser << y.sm_)) {
      return false;
    }

    return x_block.length() == y_block.length() &&
      std::memcmp(x_block.rd_ptr(), y_block.rd_ptr(), x_block.length()) == 0;
  }

  return false;
}

inline bool meta_submessage_vec_equal(const MetaSubmessageVec& x, const MetaSubmessageVec& y)
{
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), meta_submessage_equal);
}

inline MetaSubmessage create_heartbeat(const GUID_t& from, const GUID_t& dst, ACE_INT64 first, ACE_INT64 last, ACE_INT32 count, bool ignore)
{
  MetaSubmessage meta_submessage;

  const HeartBeatSubmessage heartbeat =
    {
      {
        HEARTBEAT,
        FLAG_E,
        HEARTBEAT_SZ
      },
      dst.entityId,
      from.entityId,
      to_rtps_seqnum(SequenceNumber(first)),
      to_rtps_seqnum(SequenceNumber(last)),
      { count }
    };

  meta_submessage.src_guid_ = from;
  meta_submessage.dst_guid_ = dst;
  meta_submessage.sm_.heartbeat_sm(heartbeat);
  meta_submessage.ignore_ = ignore;
  return meta_submessage;
}

inline MetaSubmessage create_acknack(const GUID_t& from, const GUID_t& dst, ACE_INT64 base, ACE_INT32 count, bool ignore)
{
  MetaSubmessage meta_submessage;

  LongSeq8 bitmap;

  const AckNackSubmessage acknack =
    {
      {
        ACKNACK,
        FLAG_E,
        0 /*length*/
      },
      from.entityId,
      dst.entityId,
      {
        to_rtps_seqnum(base),
        0 /* num_bits */,
        bitmap
      },
      {
        count
      }
    };

  meta_submessage.src_guid_ = from;
  meta_submessage.dst_guid_ = dst;
  meta_submessage.sm_.acknack_sm(acknack);
  meta_submessage.ignore_ = ignore;
  return meta_submessage;
}

inline MetaSubmessage create_gap(const GUID_t& from, const GUID_t& dst, ACE_INT64 start, ACE_INT64 base)
{
  MetaSubmessage meta_submessage;

  LongSeq8 bitmap;

  const GapSubmessage gap =
    {
      {
        GAP,
        FLAG_E,
        0 /*length*/
      },
      from.entityId,
      dst.entityId,
      to_rtps_seqnum(start),
      {
        to_rtps_seqnum(base),
        0 /* num_bits */,
        bitmap
      }
    };

  meta_submessage.src_guid_ = from;
  meta_submessage.dst_guid_ = dst;
  meta_submessage.sm_.gap_sm(gap);
  return meta_submessage;
}

const GUID_t w1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x02 } };
const GUID_t w2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x02 } };
const GUID_t r1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x07 } };
const GUID_t r2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x07 } };

}

#endif // TEST_DDS_DCPS_TRANSPORT_RTPS_UDP_UTIL_H

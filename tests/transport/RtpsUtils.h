// Utilities for writing RTPS messages directly to a socket in order to
// test the rtps_udp transport

#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/Message_Block_Ptr.h>

#include <ace/Message_Block.h>

#define INITIALIZE_GUID_PREFIX(A)                 \
  (A)[0], (A)[1], (A)[2], (A)[3], (A)[4], (A)[5], \
  (A)[6], (A)[7], (A)[8], (A)[9], (A)[10], (A)[11]

typedef std::pair<OpenDDS::RTPS::SequenceNumber_t, OpenDDS::RTPS::SequenceNumber_t> SequencePair;

inline OpenDDS::RTPS::SequenceNumber_t toSN(unsigned int n)
{
  const OpenDDS::RTPS::SequenceNumber_t sn = {0, n};
  return sn;
}

inline ACE_Message_Block*
buildHeartbeat(const OpenDDS::DCPS::EntityId_t& writer, const OpenDDS::RTPS::Header& header,
               const SequencePair& sequenceRange, int& count,
               const OpenDDS::DCPS::RepoId& reader = OpenDDS::DCPS::GUID_UNKNOWN)
{
  using namespace OpenDDS::DCPS;
  using namespace OpenDDS::RTPS;
  const InfoDestinationSubmessage infoDst = {
    {INFO_DST, FLAG_E, INFO_DST_SZ},
    {INITIALIZE_GUID_PREFIX(reader.guidPrefix)}
  };
  const HeartBeatSubmessage hb = {
    {HEARTBEAT, FLAG_E, 0},
    reader.entityId, writer, sequenceRange.first, sequenceRange.second,
    {++count}
  };
  size_t size = RTPSHDR_SZ;
  if (reader != GUID_UNKNOWN) {
    size += SMHDR_SZ + INFO_DST_SZ;
  }
  static const Encoding encoding(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
  serialized_size(encoding, size, hb);
  Message_Block_Ptr mb(new ACE_Message_Block(size));
  Serializer ser(mb.get(), encoding);
  const bool ok = (ser << header) &&
    (reader == GUID_UNKNOWN || ser << infoDst) &&
    (ser << hb);
  if (!ok) {
    mb.reset();
  }
  return mb.release();
}

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/TypeSupportImpl.h"

#include <iostream>
#include <cstring>

using OpenDDS::DCPS::KeyOnly;
using OpenDDS::DCPS::Serializer;
using namespace OpenDDS::RTPS;

struct TestMsg {
  ACE_CDR::ULong key;
  TAO::String_Manager value;
};

size_t gen_max_marshaled_size(KeyOnly<const TestMsg>, bool /*align*/)
{
  return 4;
}

void gen_find_size(KeyOnly<const TestMsg>, size_t& size, size_t& padding)
{
  if ((size + padding) % 4) {
    padding += 4 - ((size + padding) % 4);
  }
  size += 4;
}

bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru)
{
  return strm << stru.t.key;
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {
template <>
struct MarshalTraits<TestMsg> {
static bool gen_is_bounded_size() { return true; }
static bool gen_is_bounded_key_size() { return true; }
};
} }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

struct BigKey {
  CORBA::Octet key[24];
  TAO::String_Manager value;
};

size_t gen_max_marshaled_size(KeyOnly<const BigKey>, bool /*align*/)
{
  return 24;
}

void gen_find_size(KeyOnly<const BigKey>, size_t& size, size_t& /*padding*/)
{
  size += 24;
}

bool operator<<(Serializer& strm, KeyOnly<const BigKey> stru)
{
  return strm.write_octet_array(stru.t.key, 24);
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {
template <>
struct MarshalTraits<BigKey> {
inline static bool gen_is_bounded_size() { return true; }
inline static bool gen_is_bounded_key_size() { return true; }
};
} }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

bool test_key_hash()
{
  bool ok = true;

  TestMsg tm;
  tm.key = 23;
  OpenDDS::RTPS::KeyHash_t hash;
  marshal_key_hash(tm, hash);

  if (hash.value[0] || hash.value[1] || hash.value[2] || hash.value[3] != 23) {
    std::cerr << "ERROR: first 4 bytes of hash.value should be 23 (Big endian)"
              << std::endl;
    ok = false;
  }

  if (hash.value[4] || hash.value[5] || hash.value[6] || hash.value[7] ||
      hash.value[8] || hash.value[9] || hash.value[10] || hash.value[11] ||
      hash.value[12] || hash.value[13] || hash.value[14] || hash.value[15]) {
    std::cerr << "ERROR: remaining bytes of hash.value should be 0"
              << std::endl;
    ok = false;
  }

  BigKey bk;
  for (size_t i = 0; i < 24; ++i) {
    bk.key[i] = 'a' + static_cast<CORBA::Octet>(i);
  }
  OpenDDS::RTPS::KeyHash_t hash2;
  marshal_key_hash(bk, hash2);
  if (hash2.value[0] != 0x0c || hash2.value[1] != 0x0e ||
      hash2.value[2] != 0x84 || hash2.value[3] != 0xcf ||
      hash2.value[4] != 0x0b || hash2.value[5] != 0xb7 ||
      hash2.value[6] != 0xa8 || hash2.value[7] != 0x68 ||
      hash2.value[8] != 0x8e || hash2.value[9] != 0x52 ||
      hash2.value[10] != 0x38 || hash2.value[11] != 0xdb ||
      hash2.value[12] != 0xbc || hash2.value[13] != 0x1c ||
      hash2.value[14] != 0x39 || hash2.value[15] != 0x53) {
    std::cerr << "ERROR: MD5 Key hash doesn't match expected value"
              << std::endl;
    ok = false;
  }
  return ok;
}

template<typename T, size_t N>
bool test(const T& foo, const CORBA::Octet (&expected)[N],
          const char* name, bool bigendian = false)
{
  size_t size = 0, padding = 0;
  OpenDDS::DCPS::gen_find_size(foo, size, padding);
  if (size + padding != N) {
    std::cerr << "ERROR: " << name << " expected size " << N << " actual size "
                 "from gen_find_size " << size << " padding " << padding
              << std::endl;
  }
  const bool swap =
#ifdef ACE_LITTLE_ENDIAN
    bigendian;
#else
    !bigendian;
#endif
  ACE_Message_Block mb(65536);
  Serializer ser(&mb, swap, Serializer::ALIGN_INITIALIZE);
  if (!(ser << foo) || mb.length() != N) {
    std::cerr << "ERROR: " << name << " should serialize to " << N << " bytes"
                 " (actual: " << mb.length() << ')' << std::endl;
    return false;
  }
  if (std::memcmp(expected, mb.rd_ptr(), N) != 0) {
    std::cerr << "ERROR: " << name << " serialized bytes mismatched"
              << std::endl;
    return false;
  }
  Serializer ser2(&mb, swap, Serializer::ALIGN_CDR);
  T roundtrip;
  if (!(ser2 >> roundtrip) || mb.rd_ptr() != mb.wr_ptr()) {
    std::cerr << "ERROR: failed to deserialize " << name << std::endl;
    return false;
  }
  mb.reset();
  Serializer ser3(&mb, swap, Serializer::ALIGN_INITIALIZE);
  if (!(ser3 << roundtrip) || mb.length() != N
      || std::memcmp(expected, mb.rd_ptr(), N) != 0) {
    std::cerr << "ERROR: failed to reserialize " << name << std::endl;
    return false;
  }
  return true;
}

bool test_messages()
{
  const bool swap_LE = // value of the "swap" flag for Little-Endian data
#ifdef ACE_LITTLE_ENDIAN
    false;
#else
    true;
#endif
  bool ok = true;
  {
    const Header hdr = { {'R', 'T', 'P', 'S'}, PROTOCOLVERSION_2_2,
      VENDORID_OPENDDS, {0} /*GUIDPREFIX_UNKNOWN*/ };
    const CORBA::Octet expected[] = "RTPS\x02\x02\x01\x03\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00" /*includes null*/;
    ok &= test(hdr, expected, "Header");
  }
  {
    LongSeq8 bitmap;
    bitmap.length(1);
    bitmap[0] = 1;
    AckNackSubmessage sm = { {ACKNACK, 1, 28}, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, { {0, 1}, 1, bitmap}, {2} };
    const CORBA::Octet expected_LE[] = {
      6, 1, 28, 0,                // smHeader
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 1, 0, 0, 0,     // readerSNState.bitmapBase
      1, 0, 0, 0, 1, 0, 0, 0,     // readerSNState.numBits, bitmap[0]
      2, 0, 0, 0};                // count
    ok &= test(sm, expected_LE, "AckNackSubmessage");

    sm.smHeader.flags = 0;
    const CORBA::Octet expected_BE[] = {
      6, 0, 0, 28,                // smHeader
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 0, 0, 0, 1,     // readerSNState.bitmapBase
      0, 0, 0, 1, 0, 0, 0, 1,     // readerSNState.numBits, bitmap[0]
      0, 0, 0, 2};                // count
    ok &= test(sm, expected_BE, "bigendian AckNackSubmessage", true);
  }
  {
    DataSubmessage ds = { {DATA, 1, 20}, 0, 16, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, {0, 2}, ParameterList() };
    const CORBA::Octet expected[] = {
      21, 1, 20, 0,               // smHeader
      0, 0, 16, 0,                // extraFlags, octetsToInlineQos
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 2, 0, 0, 0};    // writerSN
    ok &= test(ds, expected, "DataSubmessage");
    // enable InlineQosFlag
    ds.smHeader.flags |= 2;
    ParameterList iqos(3);
    iqos.length(3);
    iqos[0].string_data("my_topic_name"); // 14 with null
    iqos[0]._d(PID_TOPIC_NAME);
    iqos[1].string_data("my_type_name"); // 13 with null
    iqos[1]._d(PID_TYPE_NAME);
    OpenDDS::DCPS::ContentFilterProperty_t cfprop; // 4 empty strings + 1 empty StringSeq
    iqos[2].content_filter_property(cfprop);
    ds.inlineQos = iqos;
    ds.smHeader.submessageLength = 112;
    const CORBA::Octet expected_iqos[] = {
      21, 3, 112, 0,              // smHeader
      0, 0, 16, 0,                // extraFlags, octetsToInlineQos
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 2, 0, 0, 0,     // writerSN
      5, 0, 20, 0, 14, 0, 0, 0,   // PID_TOPIC_NAME, param len, string len
      'm', 'y', '_', 't', 'o', 'p', 'i', 'c', '_', 'n', 'a', 'm', 'e', 0, 0, 0,
      7, 0, 20, 0, 13, 0, 0, 0,   // PID_TYPE_NAME, param len, string len
      'm', 'y', '_', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e', 0, 0, 0, 0,
      53, 0, 36, 0,               // PID_CONTENT_FILTER_PROPERTY, param len
      1, 0, 0, 0, 0, 0, 0, 0,     // contentFilteredTopicName
      1, 0, 0, 0, 0, 0, 0, 0,     // relatedTopicName
      1, 0, 0, 0, 0, 0, 0, 0,     // filterClassName
      1, 0, 0, 0, 0, 0, 0, 0,     // filterExpression
      0, 0, 0, 0,                 // expressionParameters
      1, 0, 0, 0};                // PID_SENTINEL, ignored
    ok &= test(ds, expected_iqos, "DataSubmessage with inlineQos");
    ds.smHeader.flags &= ~1;
    const CORBA::Octet expected_iqos_BE[] = {
      21, 2, 0, 112,              // smHeader
      0, 0, 0, 16,                // extraFlags, octetsToInlineQos
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 0, 0, 0, 2,     // writerSN
      0, 5, 0, 20, 0, 0, 0, 14,   // PID_TOPIC_NAME, param len, string len
      'm', 'y', '_', 't', 'o', 'p', 'i', 'c', '_', 'n', 'a', 'm', 'e', 0, 0, 0,
      0, 7, 0, 20, 0, 0, 0, 13,   // PID_TYPE_NAME, param len, string len
      'm', 'y', '_', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e', 0, 0, 0, 0,
      0, 53, 0, 36,               // PID_CONTENT_FILTER_PROPERTY, param len
      0, 0, 0, 1, 0, 0, 0, 0,     // contentFilteredTopicName
      0, 0, 0, 1, 0, 0, 0, 0,     // relatedTopicName
      0, 0, 0, 1, 0, 0, 0, 0,     // filterClassName
      0, 0, 0, 1, 0, 0, 0, 0,     // filterExpression
      0, 0, 0, 0,                 // expressionParameters
      0, 1, 0, 0};                // PID_SENTINEL, ignored
    ok &= test(ds, expected_iqos_BE, "BE DataSubmessage with inlineQos", true);
    const CORBA::Octet input_large_otiq[] = {
      21, 3, 92, 0,               // smHeader
      0, 0, 36, 0,                // extraFlags, octetsToInlineQos
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 2, 0, 0, 0,     // writerSN
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // unknown
      5, 0, 20, 0, 14, 0, 0, 0,   // PID_TOPIC_NAME, param len, string len
      'm', 'y', '_', 't', 'o', 'p', 'i', 'c', '_', 'n', 'a', 'm', 'e', 0, 0, 0,
      7, 0, 20, 0, 13, 0, 0, 0,   // PID_TYPE_NAME, param len, string len
      'm', 'y', '_', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e', 0, 0, 0, 0,
      1, 0, 0, 0};                // PID_SENTINEL, ignored
    ACE_Message_Block mb(reinterpret_cast<const char*>(input_large_otiq),
                         sizeof(input_large_otiq));
    mb.wr_ptr(sizeof(input_large_otiq));
    Serializer ser(&mb, swap_LE, Serializer::ALIGN_CDR);
    DataSubmessage ds2;
    if (!(ser >> ds2) || mb.length() || ds2.inlineQos.length() != 2
        || ds2.inlineQos[0]._d() != PID_TOPIC_NAME
        || ds2.inlineQos[1]._d() != PID_TYPE_NAME
        || 0 != std::strcmp(ds2.inlineQos[0].string_data(), "my_topic_name")
        || 0 != std::strcmp(ds2.inlineQos[1].string_data(), "my_type_name")) {
      std::cerr << "ERROR: failed to deserialize DataSubmessage with larger "
                   "than normal value of octetsToInlineQos.\n";
      ok = false;
    }
  }
  {
    DataFragSubmessage df = { {DATA_FRAG, 1, 1056}, 0, 28, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, {0, 3}, {4}, 1, 1024, 1493,
      ParameterList()};
    const CORBA::Octet expected[] = {
      22, 1, 32, 4,               // smHeader
      0, 0, 28, 0,                // extraFlags, octetsToInlineQos
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 3, 0, 0, 0,     // writerSN
      4, 0, 0, 0, 1, 0, 0, 4,     // fragStartingNum, fragInSub, fragSize
      213, 5, 0, 0};              // sampleSize
    ok &= test(df, expected, "DataFragSubmessage");
    const CORBA::Octet input_large_otiq[] = {
      22, 1, 32, 4,               // smHeader
      0, 0, 40, 0,                // extraFlags, octetsToInlineQos
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 3, 0, 0, 0,     // writerSN
      4, 0, 0, 0, 1, 0, 0, 4,     // fragStartingNum, fragInSub, fragSize
      213, 5, 0, 0,               // sampleSize
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // unknown
    ACE_Message_Block mb(reinterpret_cast<const char*>(input_large_otiq),
                         sizeof(input_large_otiq));
    mb.wr_ptr(sizeof(input_large_otiq));
    Serializer ser(&mb, swap_LE, Serializer::ALIGN_CDR);
    DataFragSubmessage df2;
    if (!(ser >> df2) || mb.length()) {
      std::cerr << "ERROR: failed to deserialize DataSubmessage with larger "
                   "than normal value of octetsToInlineQos.\n";
      ok = false;
    }
  }
  {
    LongSeq8 bitmap;
    bitmap.length(2);
    bitmap[0] = 0xFEDCBA98;
    bitmap[1] = 0x76543210;
    GapSubmessage gs = { {GAP, 1, 36}, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, {0, 4}, { {0, 6}, 33, bitmap } };
    const CORBA::Octet expected[] = {
      8, 1, 36, 0,                // smHeader
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 4, 0, 0, 0,     // gapStart
      0, 0, 0, 0, 6, 0, 0, 0,     // gapList.bitmapBase
      33, 0, 0, 0, 152, 186, 220, 254, // gapList.numBits, gapList.bitmap[0]
      16, 50, 84, 118};                // gapList.bitmap[1]
    ok &= test(gs, expected, "GapSubmessage");
  }
  {
    HeartBeatSubmessage hbs = { {HEARTBEAT, 1, 28}, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, {0, 7}, {0, 8}, {9} };
    const CORBA::Octet expected[] = {
      7, 1, 28, 0,                // smHeader
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 7, 0, 0, 0,     // firstSN
      0, 0, 0, 0, 8, 0, 0, 0,     // lastSN
      9, 0, 0, 0};                // count
    ok &= test(hbs, expected, "HeartBeatSubmessage");
  }
  {
    HeartBeatFragSubmessage hbf = { {HEARTBEAT_FRAG, 1, 24}, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, {0, 10}, {11}, {12} };
    const CORBA::Octet expected[] = {
      19, 1, 24, 0,               // smHeader
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 10, 0, 0, 0,    // writerSN
      11, 0, 0, 0, 12, 0, 0, 0};  // lastFragmentNum, count
    ok &= test(hbf, expected, "HeartBeatFragSubmessage");
  }
  {
    InfoDestinationSubmessage id = { {INFO_DST, 1, 12}, {1, 2, 3, 4, 5, 6, 7, 8,
      9, 10, 11, 12} };
    const CORBA::Octet expected[] = {
      14, 1, 12, 0,               // smHeader
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // guidPrefix
    ok &= test(id, expected, "InfoDestinationSubmessage");
  }
  {
    OpenDDS::DCPS::Locator_t loc[] = {
      {LOCATOR_KIND_UDPv4, 49152, {0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 10, 1, 0, 1} },
      {LOCATOR_KIND_UDPv6, 49153, {0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 255, 255, 10, 1, 0, 1} },
      {LOCATOR_KIND_UDPv6, 49154, {1, 32, 184, 13, 163, 133,
                                   0, 0, 0, 0, 46, 138, 112, 3, 52, 115} } };
    LocatorList unicastLocatorList(sizeof(loc)/sizeof(loc[0]), loc);
    InfoReplySubmessage ir = { {INFO_REPLY, 1, 64}, unicastLocatorList,
      LocatorList()};
    const CORBA::Octet expected[] = {
      15, 1, 64, 0,               // smHeader
      3, 0, 0, 0,                 // uniLL.length
      1, 0, 0, 0, 0, 192, 0, 0,   // uniLL[0].kind, uniLL[0].port
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1, 0, 1, // uniLL[0].addr
      2, 0, 0, 0, 1, 192, 0, 0,   // uniLL[1].kind, uniLL[1].port
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 10, 1, 0, 1, // uniLL[1].addr
      2, 0, 0, 0, 2, 192, 0, 0,   // uniLL[2].kind, uniLL[2].port
      1, 32, 184, 13, 163, 133, 0, 0, 0, 0, 46, 138, 112, 3, 52, 115};
    ok &= test(ir, expected, "InfoReplySubmessage");
    OpenDDS::DCPS::Locator_t multi[] = {
      {LOCATOR_KIND_UDPv4, 49155, {0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 224, 0, 0, 1} },
      {LOCATOR_KIND_UDPv6, 49156, {2, 255, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 1} } };
    LocatorList multicastLocatorList(sizeof(multi)/sizeof(multi[0]), multi);
    ir.smHeader.flags |= 2;
    ir.multicastLocatorList = multicastLocatorList;
    ir.smHeader.submessageLength = 108;
    const CORBA::Octet expected_mc[] = {
      15, 3, 108, 0,              // smHeader
      3, 0, 0, 0,                 // uniLL.length
      1, 0, 0, 0, 0, 192, 0, 0,   // uniLL[0].kind, uniLL[0].port
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1, 0, 1, // uniLL[0].addr
      2, 0, 0, 0, 1, 192, 0, 0,   // uniLL[1].kind, uniLL[1].port
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 10, 1, 0, 1, // uniLL[1].addr
      2, 0, 0, 0, 2, 192, 0, 0,   // uniLL[2].kind, uniLL[2].port
      1, 32, 184, 13, 163, 133, 0, 0, 0, 0, 46, 138, 112, 3, 52, 115,
      2, 0, 0, 0,                 // multiLL.length
      1, 0, 0, 0, 3, 192, 0, 0,   // multiLL[0].kind, multiLL[0].port
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224, 0, 0, 1, // multiLL[0].addr
      2, 0, 0, 0, 4, 192, 0, 0,   // multiLL[1].kind, multiLL[1].port
      2, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}; // multiLL[1].addr
    ok &= test(ir, expected_mc, "InfoReplySubmessage with multicast");
  }
  {
    InfoSourceSubmessage is = { {INFO_SRC, 1, 20}, 0, {2, 1}, { {1, 3} },
      {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1} };
    const CORBA::Octet expected[] = {
      12, 1, 20, 0,               // smHeader
      0, 0, 0, 0, 2, 1, 1, 3,     // unused, version, vendorId
      12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}; // guidPrefix
    ok &= test(is, expected, "InfoSourceSubmessage");
  }
  {
    InfoTimestampSubmessage it = { {INFO_TS, 1, 8}, {1315413839, 822079774} };
    const CORBA::Octet expected[] = {
      9, 1, 8, 0,                          // smHeader
      79, 159, 103, 78, 30, 241, 255, 48}; // timestamp
    ok &= test(it, expected, "InfoTimestampSubmessage");
    it.smHeader.flags |= 2;
    it.smHeader.submessageLength = 0;
    const CORBA::Octet expected_no_ts[] = {9, 3, 0, 0};
    ok &= test(it, expected_no_ts, "InfoTimestampSubmessage without timestamp");
  }
  {
    PadSubmessage pad = { {PAD, 1, 0} };
    const CORBA::Octet expected[] = {1, 1, 0, 0};
    ok &= test(pad, expected, "PadSubmessage");
  }
  {
    CORBA::Long bits[] = {0, 0, 0, 0, 0, 0, 0, -2147483647 - 1};
    LongSeq8 bitmap(8, bits);
    NackFragSubmessage nf = { {NACK_FRAG, 1, 60}, ENTITYID_UNKNOWN,
      ENTITYID_SEDP_BUILTIN_TOPIC_WRITER, {0, 11}, { {12}, 256, bitmap}, {12} };
    const CORBA::Octet expected[] = {
      18, 1, 60, 0,               // smHeader
      0, 0, 0, 0, 0, 0, 2, 194,   // readerId, writerId
      0, 0, 0, 0, 11, 0, 0, 0,    // writerSN
      12, 0, 0, 0, 0, 1, 0, 0,    // fragmentNumberState.{bitmapBase,numBits}
      0, 0, 0, 0, 0, 0, 0, 0,     // fragmentNumberState.bitmap[0,1]
      0, 0, 0, 0, 0, 0, 0, 0,     // fragmentNumberState.bitmap[2,3]
      0, 0, 0, 0, 0, 0, 0, 0,     // fragmentNumberState.bitmap[4,5]
      0, 0, 0, 0, 0, 0, 0, 128,   // fragmentNumberState.bitmap[6,7]
      12, 0, 0, 0};               // count
    ok &= test(nf, expected, "NackFragSubmessage");
  }
  {
    InfoReplyIp4Submessage iri4 = { {INFO_REPLY_IP4, 1, 8},
      {167837697, 49157}, LocatorUDPv4_t()};
    const CORBA::Octet expected[] = {
      13, 1, 8, 0,                // smHeader
      1, 0, 1, 10, 5, 192, 0, 0}; // unicastLocator
    ok &= test(iri4, expected, "InfoReplyIp4Submessage");
    iri4.smHeader.flags |= 2;
    iri4.smHeader.submessageLength = 16;
    iri4.multicastLocator.address = 3758096385u;
    iri4.multicastLocator.port = 49158;
    const CORBA::Octet expected_mc[] = {
      13, 3, 16, 0,                // smHeader
      1, 0, 1, 10, 5, 192, 0, 0,   // unicastLocator
      1, 0, 0, 224, 6, 192, 0, 0}; // multicastLocator
    ok &= test(iri4, expected_mc, "multicast InfoReplyIp4Submessage");
  }
  return ok;
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  bool ok = true;
  ok &= test_key_hash();
  ok &= test_messages();
  return ok ? 0 : 1;
}

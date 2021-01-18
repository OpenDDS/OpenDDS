#ifndef XTYPES_H
#define XTYPES_H

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

bool verbose = false;
bool expect_to_match = true;

int key_value = -1;

enum AdditionalFieldValue {
  FINAL_STRUCT_AF,
  APPENDABLE_STRUCT_AF,
  MUTABLE_STRUCT_AF,
  NESTED_STRUCT_AF
};

const std::string STRING_26 = "abcdefghijklmnopqrstuvwxyz";
const std::string STRING_20 = "abcdefghijklmnopqrst";

#endif

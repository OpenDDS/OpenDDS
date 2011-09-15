#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Serializer.h"

#include <tao/CORBA_String.h>

#include <ace/OS_main.h>
#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/String_Base.h>
#include <ace/Get_Opt.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Message_Block.h>

#include <iostream>
#include <sstream>
#include <cstring>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

struct TestMsg {
  ACE_CDR::ULong key;
  TAO::String_Manager value;
};

bool gen_is_bounded_size(KeyOnly<const TestMsg>)
{
  return true;
}

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

// sample text (pasted directly from the RTPS spec) to use in the message data
const char text[] = "Implementation of the protocol that are processing a "
  "received submessage should always use the octetsToInlineQos to skip "
  "any submessage headers it does not expect or understand and continue to "
  "process the inlineQos SubmessageElement (or the first submessage element "
  "that follows inlineQos if the inlineQos is not present). This fule is "
  "necessary so that the received will be able to interoperate with senders "
  "that use future versions of the protocol which may include additional "
  "submessage headers before the inlineQos.\n";

const bool host_is_bigendian =
#ifdef ACE_LITTLE_ENDIAN
    false;
#else
    true;
#endif

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ACE_TString host;
  u_short port;

  ACE_Get_Opt opts(argc, argv, ACE_TEXT("h:p:"));
  int option = 0;

  while ((option = opts()) != EOF) {
    switch (option) {
    case 'h':
      host = opts.opt_arg();
      break;
    case 'p':
      port = static_cast<u_short>(ACE_OS::atoi(opts.opt_arg()));
      break;
    }
  }

  if (host.empty() || port == 0) {
    std::cerr << "ERROR: -h <host> and -p <port> options are required\n";
    return 1;
  }

  ACE_INET_Addr local_addr;
  ACE_SOCK_Dgram sock(local_addr);
  ACE_INET_Addr remote_addr(port, host.c_str());

  RepoIdBuilder local;
  local.federationId(0x01234567);  // guidPrefix1
  local.participantId(0x89abcdef); // guidPrefix2
  local.entityKey(0x012345);
  local.entityKind(ENTITYKIND_USER_WRITER_WITH_KEY);
  OpenDDS::RTPS::GUID_t local_guid(local);
  const OpenDDS::RTPS::GuidPrefix_t& local_prefix = local_guid.guidPrefix;

  const Header hdr = { {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
    {local_prefix[0], local_prefix[1], local_prefix[2], local_prefix[3],
     local_prefix[4], local_prefix[5], local_prefix[6], local_prefix[7],
     local_prefix[8], local_prefix[9], local_prefix[10], local_prefix[11]} };

  const InfoTimestampSubmessage it = { {INFO_TS, 1, 8},
                                       {1315413839, 822079774} };

  DataSubmessage ds = { {DATA, 7, 20 + 24 + 12 + sizeof(text)}, 0, 16,
    ENTITYID_UNKNOWN, local_guid.entityId, {0, 1}, ParameterList() };

  TestMsg data;
  data.key = 0x09230923;
  data.value = text;

  ds.inlineQos.length(1);
  OpenDDS::RTPS::KeyHash_t hash;
  marshal_key_hash(data, hash);
  ds.inlineQos[0].key_hash_(hash);

  size_t size = 0, padding = 0;
  gen_find_size(hdr, size, padding);
  gen_find_size(it, size, padding);
  gen_find_size(ds, size, padding);
  size += 12 + sizeof(text);

  ACE_Message_Block msg(size + padding);
  Serializer ser(&msg, host_is_bigendian, Serializer::ALIGN_CDR);
  const ACE_CDR::ULong encap = 0x00000100; // {CDR_LE, options} in BE format
  const bool ok = (ser << hdr) && (ser << it) && (ser << ds)
    && (ser << encap) && (ser << data.key) && (ser << data.value);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize RTPS message\n";
    return 1;
  }

  ssize_t res = sock.send(msg.rd_ptr(), msg.length(), remote_addr);
  if (res < 0) {
    std::cerr << "ERROR: error in send()\n";
    return 1;
  } else {
    std::ostringstream oss;
    oss << "Sent " << res << " bytes.\n";
    ACE_DEBUG((LM_INFO, oss.str().c_str()));
  }

  return 0;
}

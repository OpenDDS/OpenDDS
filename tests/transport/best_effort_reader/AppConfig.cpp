#include "AppConfig.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include <ace/Basic_Types.h>
#include <ace/Get_Opt.h>
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

const OpenDDS::DCPS::RepoId AppConfig::writerId[3] = {
  createID(0x11111111, 0x111111, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY),
  createID(0x11111111, 0x222222, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY),
  createID(0x11111111, 0x333333, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY)
};

const OpenDDS::DCPS::RepoId AppConfig::readerId[3] = {
  createID(0x22222222, 0x111111, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY),
  createID(0x22222222, 0x222222, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY),
  createID(0x22222222, 0x333333, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY)
};

AppConfig::AppConfig(int argc, ACE_TCHAR* argv[]) :
  dpf(TheParticipantFactoryWithArgs(argc, argv)), host(ACE_TEXT("")), port(0)
{
  try {
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
#ifdef OPENDDS_SAFETY_PROFILE
    if (host == ACE_TEXT("localhost")) {
      host = "127.0.0.1";
    }
#endif
    if (host.empty() || port == 0) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("ERROR: -h <host> and -p <port> are missing\n")));
      throw std::string("-h <host> and -p <port> missing");
    }
  } catch (...) {
    cleanup();
    throw;
  }
}

AppConfig::~AppConfig() {
  cleanup();
}

ACE_INET_Addr AppConfig::getHostAddress() const {
  return ACE_INET_Addr(port, host.c_str(),
    (host == ACE_TEXT("localhost")) ? AF_UNSPEC : AF_INET);
}

OpenDDS::DCPS::RepoId AppConfig::createID(long participantId, long key, CORBA::Octet kind) {
  OpenDDS::DCPS::RepoIdBuilder idBd;
  idBd.federationId(0x01234567); // guidPrefix1
  idBd.participantId(participantId); // guidPrefix2
  idBd.entityKey(key);
  idBd.entityKind(kind);
  return idBd;
}

void AppConfig::to_cerr(const OpenDDS::DCPS::RepoId& remote, const OpenDDS::DCPS::RepoId& local, const std::string& txt) const {
  std::cerr << OPENDDS_STRING(OpenDDS::DCPS::GuidConverter(remote)) << " <- "
            << OPENDDS_STRING(OpenDDS::DCPS::GuidConverter(local)) << " " << txt << std::endl;
}

void AppConfig::cleanup() {
  if (dpf) {
    TheServiceParticipant->shutdown();
    dpf = 0;
  }
}

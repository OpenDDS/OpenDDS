#include "AppConfig.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include <ace/Basic_Types.h>
#include <ace/Get_Opt.h>
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

AppConfig::AppConfig(int argc, ACE_TCHAR* argv[], bool setLocalAddress) :
  dpf(TheParticipantFactoryWithArgs(argc, argv)),
  host(ACE_TEXT("")), port(0), rtpsInst(0)
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
      throw;
    }

    transportInst = TheTransportRegistry->create_inst("my_rtps", "rtps_udp");
    rtpsInst = dynamic_cast<OpenDDS::DCPS::RtpsUdpInst*>(transportInst.in());
    if (!rtpsInst) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("ERROR: Failed to cast to RtpsUdpInst*\n")));
      throw;
    }
    rtpsInst->datalink_release_delay_ = 0;
    if (setLocalAddress) {
      rtpsInst->local_address(port, ACE_TEXT_ALWAYS_CHAR(host.c_str()));
    }

    OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
    cfg->instances_.push_back(transportInst);
    TheTransportRegistry->global_config(cfg);

    pubWtrId[0] = createID(0x11111111, 0x111111, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);
    pubWtrId[1] = createID(0x11111111, 0x222222, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);
    pubWtrId[2] = createID(0x11111111, 0x333333, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);
    subRdrId[0] = createID(0x22222222, 0x111111, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY);
    subRdrId[1] = createID(0x22222222, 0x222222, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY);
    subRdrId[2] = createID(0x22222222, 0x333333, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY);
  } catch (const OpenDDS::DCPS::Transport::Exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("EXCEPTION: %C\n"), typeid(e).name()));
    cleanup();
    throw;
  } catch (const CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("EXCEPTION: %C\n"), e._info().c_str()));
    cleanup();
    throw;
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

const OpenDDS::DCPS::TimeDuration& AppConfig::getHeartbeatPeriod() const {
  return rtpsInst->heartbeat_period_;
}
void AppConfig::setHeartbeatPeriod(const ACE_UINT64& ms) {
  rtpsInst->heartbeat_period_ = OpenDDS::DCPS::TimeDuration::from_msec(ms);
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

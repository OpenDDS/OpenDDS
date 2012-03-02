
#include "Application.h"

#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/udp/UdpInst.h"
#include "dds/DCPS/transport/multicast/MulticastInst.h"

#include "dds/DCPS/Service_Participant.h"

OpenDDS::Model::Application::Application(int& argc, ACE_TCHAR *argv[])
  : factory_(TheParticipantFactoryWithArgs(argc, argv))
{
}

OpenDDS::Model::Application::~Application()
{
  CORBA::release(factory_);
  TheServiceParticipant->shutdown();
}


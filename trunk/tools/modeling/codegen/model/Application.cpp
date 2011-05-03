
#include "Application.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"

#include "dds/DCPS/Service_Participant.h"

OpenDDS::Model::Application::Application(int& argc, ACE_TCHAR *argv[])
  : factory_(TheParticipantFactoryWithArgs(argc, argv))
{
}

OpenDDS::Model::Application::~Application()
{
  CORBA::release(factory_);
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}


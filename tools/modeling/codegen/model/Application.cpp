
#include "Application.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"

#include "dds/DCPS/Service_Participant.h"

OpenDDS::Model::Application::Application(int& argc, char** argv) 
{
  TheParticipantFactoryWithArgs(argc, argv);
}

OpenDDS::Model::Application::~Application()
{
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}


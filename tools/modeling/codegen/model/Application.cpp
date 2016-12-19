
#include "Application.h"
#include "dds/DCPS/Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::Application::Application(int& argc, ACE_TCHAR *argv[])
  : factory_(TheParticipantFactoryWithArgs(argc, argv))
{
}

OpenDDS::Model::Application::~Application()
{
  CORBA::release(factory_);
  TheServiceParticipant->shutdown();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

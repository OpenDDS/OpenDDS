#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

#include "MultiTopicTestTypeSupportImpl.h"

using namespace DDS;
using namespace OpenDDS::DCPS;

int run_test(int argc, ACE_TCHAR *argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(23, PARTICIPANT_QOS_DEFAULT, 0,
                            DEFAULT_STATUS_MASK);
  ResultingTypeSupport_var ts = new ResultingTypeSupportImpl;
  ts->register_type(dp, "");
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int ret = run_test(argc, argv);
  TheTransportFactory->release();
  TheServiceParticipant->shutdown ();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}

#include "DCPSDataWriterI.h"
#include "DCPSDataReaderI.h"
#include "DCPSSubscriberI.h"

#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"
#include "dds/DCPS/InfoRepoDiscovery/InfoC.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/MonitorFactory.h"

#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/XTypes/TypeObject.h"

#include "tao/PortableServer/PortableServer.h"

#include "ace/Arg_Shifter.h"
#include "ace/Argv_Type_Converter.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <string>
#include <cstring>

const ACE_TCHAR* ior = ACE_TEXT("file://dcps_ir.ior");
bool qos_tests = false;
bool use_rtps = false;
bool failed = false;

class DDS_TEST
{
public:
  static void set_part_bit_subscriber(OpenDDS::DCPS::Discovery_rch disc, DDS::DomainId_t domain,
    OpenDDS::DCPS::RepoId partId, const DDS::Subscriber_var& bit_subscriber)
  {
    OpenDDS::RTPS::RtpsDiscovery* rtpsDisc = dynamic_cast<OpenDDS::RTPS::RtpsDiscovery*>(disc.in());
    if (!rtpsDisc) {
      std::cerr << "ERROR: Could not cast to RtpsDiscovery\n";
      return;
    }
    rtpsDisc->get_part(domain, partId)->init_bit(bit_subscriber);
  }
};

int parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left())
  {
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0) {
      ior = currentArg;
      arg_shifter.consume_arg();

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-q")) == 0) {
      qos_tests = true;
      arg_shifter.consume_arg();

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-r")) == 0) {
      use_rtps = true;
      arg_shifter.consume_arg();

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        "usage:  %s [-r|-k <ior>]\n"
                        "        -i (ignore tests)\n"
                        "        -q (incompatible qos test)\n"
                        "        -? (usage message)\n",
                        argv[0]),
                       -1);
    } else {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

bool pubsub(OpenDDS::DCPS::Discovery_rch disc, CORBA::ORB_var orb)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("pubsub test\n")));

  CORBA::Long domain = 9;

  OpenDDS::DCPS::RepoId pubPartId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId pubTopicId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId pubId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RcHandle<TAO_DDS_DCPSDataWriter_i> dwImpl(OpenDDS::DCPS::make_rch<TAO_DDS_DCPSDataWriter_i>());

  DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();

  ::DDS::DomainParticipantQos_var partQos = new ::DDS::DomainParticipantQos;
  *partQos = TheServiceParticipant->initial_DomainParticipantQos();
  OpenDDS::DCPS::AddDomainStatus value = disc->add_domain_participant(domain, partQos.in());
  pubPartId = value.id;
  if (OpenDDS::DCPS::GUID_UNKNOWN == pubPartId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_domain_participant failed!\n") ));
    }

  DDS::Subscriber_var sub;
  DDS::Subscriber_var sub2;
  if (use_rtps) {
    sub = new TAO_DDS_DCPSSubscriber_i;
    DDS_TEST::set_part_bit_subscriber(disc, domain, pubPartId, sub);
  }

  ::DDS::TopicQos_var topicQos = new ::DDS::TopicQos;
  *topicQos = TheServiceParticipant->initial_TopicQos();

  struct Callbacks : public OpenDDS::DCPS::TopicCallbacks {
    void inconsistent_topic(int /*count*/) {}
  } callbacks;

  if (use_rtps) { // check that topic/type name string bounds are enforced
    const std::string longname(300, 'a');
    OpenDDS::DCPS::RepoId topicId;
    const bool key = false;
    OpenDDS::DCPS::TopicStatus ts =
      disc->assert_topic(topicId, domain, pubPartId,
                         longname.c_str(), "shortname", topicQos, key, &callbacks);
    if (ts != OpenDDS::DCPS::PRECONDITION_NOT_MET) {
      failed = true;
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: expected long topic name to be rejected\n")));
    }
    ts = disc->assert_topic(topicId, domain, pubPartId,
                            "shortname", longname.c_str(), topicQos, key, &callbacks);
    if (ts != OpenDDS::DCPS::PRECONDITION_NOT_MET) {
      failed = true;
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: expected long type name to be rejected\n")));
    }
  }

  // add a topic
  const char* tname = "MYtopic";
  const char* dname = "MYdataname";

  OpenDDS::DCPS::TopicStatus topicStatus = disc->assert_topic(pubTopicId,
                                                              domain,
                                                              pubPartId,
                                                              tname,
                                                              dname,
                                                              topicQos.in(),
                                                              false,
                                                              &callbacks);

  if (topicStatus != OpenDDS::DCPS::CREATED)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Topic creation failed and returned %d"), topicStatus));
    }

  // Add publication
  if (!dwImpl->received().expectNothing())
    {
      failed = true;
    }

  ::DDS::DataWriterQos_var dwQos = new ::DDS::DataWriterQos;
  *dwQos = TheServiceParticipant->initial_DataWriterQos();
  dwQos->reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  OpenDDS::DCPS::TransportLocatorSeq tii;
  tii.length(1);
  tii[0].transport_type = "fake transport for test";

  OpenDDS::XTypes::TypeInformation type_info;
  type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
  type_info.minimal.dependent_typeid_count = 0;
  type_info.complete.dependent_typeid_count = 0;

  ::DDS::PublisherQos_var pQos = new ::DDS::PublisherQos;
  *pQos = TheServiceParticipant->initial_PublisherQos();
  pubId = disc->add_publication(domain,
                                pubPartId,
                                pubTopicId,
                                rchandle_from(dwImpl.in()),
                                dwQos.in(),
                                tii,
                                pQos.in(),
                                type_info);
  if (OpenDDS::DCPS::GUID_UNKNOWN == pubId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_publication failed!\n") ));
    }

  if (!dwImpl->received().expectNothing())
    {
      failed = true;
    }

  // add an inconsistent topic
  OpenDDS::DCPS::RepoId topicId2;
  const char* tnameIncompatible = "MYtopic";
  const char* dnameIncompatible = "MYnewdataname";
  ::DDS::TopicQos_var topicQosIncompatible = new ::DDS::TopicQos;
  *topicQosIncompatible = TheServiceParticipant->initial_TopicQos();
  topicStatus = disc->assert_topic(topicId2,
                                   domain,
                                   pubPartId,
                                   tnameIncompatible,
                                   dnameIncompatible,
                                   topicQosIncompatible.in(),
                                   false,
                                   0);

  if (topicStatus != OpenDDS::DCPS::CONFLICTING_TYPENAME)
    {
      failed = true;
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: Inconsistent topic creation did not fail with ")
                 ACE_TEXT("CONFLICTING_TYPENAME and returned %d"), topicStatus));
    }

  if (!dwImpl->received().expectNothing())
    {
      failed = true;
    }

  OpenDDS::DCPS::RepoId subPartId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId subTopicId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId subId = OpenDDS::DCPS::GUID_UNKNOWN;
  TAO_DDS_DCPSDataReader_i drImpl;
  if (use_rtps)
    drImpl.disco_ = disc.in();

  value = disc->add_domain_participant(domain, partQos.in());
  subPartId = value.id;
  if( OpenDDS::DCPS::GUID_UNKNOWN == subPartId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_domain_participant failed to creating new participant for same domain!\n") ));
    }
  if( subPartId == pubPartId )
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_domain_participant returned existing participant!\n") ));
    }

  if (use_rtps) {
    sub2 = new TAO_DDS_DCPSSubscriber_i;
    DDS_TEST::set_part_bit_subscriber(disc, domain, subPartId, sub2);
  }

  topicQos = new ::DDS::TopicQos;
  *topicQos = TheServiceParticipant->initial_TopicQos();
  topicStatus = disc->assert_topic(subTopicId,
                                   domain,
                                   subPartId,
                                   tname,
                                   dname,
                                   topicQos.in(),
                                   false,
                                   &callbacks);

  if (topicStatus != OpenDDS::DCPS::CREATED)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Topic creation failed and returned %d"), topicStatus));
    }
  if (subTopicId == pubTopicId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Topic creation returned existing topic")));
    }

  if (!dwImpl->received().expectNothing())
    {
      failed = true;
    }

  drImpl.domainId_ = domain;
  drImpl.participantId_ = subPartId;

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("adding matching subscription\n")));
  // Add subscription
  ::DDS::DataReaderQos_var drQos = new ::DDS::DataReaderQos;
  *drQos = TheServiceParticipant->initial_DataReaderQos();
  drQos->reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;

  ::DDS::SubscriberQos_var subQos = new ::DDS::SubscriberQos;
  *subQos = TheServiceParticipant->initial_SubscriberQos();

  type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
  type_info.minimal.dependent_typeid_count = 0;
  type_info.complete.dependent_typeid_count = 0;

  subId = disc->add_subscription(domain,
                                 subPartId,
                                 subTopicId,
                                 rchandle_from(&drImpl),
                                 drQos.in(),
                                 tii,
                                 subQos.in(),
                                 "", "", DDS::StringSeq(),
                                 type_info);
  if( OpenDDS::DCPS::GUID_UNKNOWN == subId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_subscription failed!\n") ));
    }

  std::vector<DiscReceivedCalls::Called> expected;
  expected.push_back(DiscReceivedCalls::ADD_ASSOC);

  unsigned int max_delay = 10;
  if (!drImpl.received().expect(orb, max_delay, expected))
    {
      failed = true;
    }

  if (!dwImpl->received().expect(orb, max_delay, expected))
    {
      failed = true;
    }

  expected.clear();
  expected.push_back(DiscReceivedCalls::ADD_ASSOC);


  // incompatible QOS
  OpenDDS::DCPS::RepoId pubIncQosId = OpenDDS::DCPS::GUID_UNKNOWN;
  TAO_DDS_DCPSDataWriter_i dwIncQosImpl;

  // Add publication
  if (!dwIncQosImpl.received().expectNothing())
    {
      failed = true;
    }

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("adding incompatible publication\n")));
  ::DDS::DataWriterQos_var dwIncQosQos = new ::DDS::DataWriterQos;
  *dwIncQosQos = TheServiceParticipant->initial_DataWriterQos();
  dwIncQosQos->reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;

  pQos = new ::DDS::PublisherQos;
  *pQos = TheServiceParticipant->initial_PublisherQos();

  type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
  type_info.minimal.dependent_typeid_count = 0;
  type_info.complete.dependent_typeid_count = 0;

  pubIncQosId = disc->add_publication(domain,
                                pubPartId,
                                pubTopicId,
                                rchandle_from(&dwIncQosImpl),
                                dwIncQosQos.in(),
                                tii,
                                pQos.in(),
                                type_info);
  if (OpenDDS::DCPS::GUID_UNKNOWN == pubIncQosId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_publication failed!\n") ));
    }

  expected.clear();
  expected.push_back(DiscReceivedCalls::UPDATE_INCOMP_QOS);

  if (!dwIncQosImpl.received().expect(orb, max_delay, expected))
    {
      failed = true;
    }

  if (!drImpl.received().expect(orb, max_delay, expected))
    {
      failed = true;
    }

  disc->remove_publication(domain, pubPartId, pubIncQosId);
  disc->remove_subscription(domain, subPartId, subId);
  disc->remove_publication(domain, pubPartId, pubId);
  disc->remove_topic(domain, pubPartId, pubTopicId);
  disc->remove_topic(domain, subPartId, subTopicId);
  disc->remove_domain_participant(domain, subPartId);
  disc->remove_domain_participant(domain, pubPartId);

  return true;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  if (parse_args(argc, argv) != 0)
    return 1;

  try
    {
      ACE_Argv_Type_Converter converter(argc, argv);

      CORBA::ORB_var orb =
        CORBA::ORB_init(converter.get_argc(), converter.get_ASCII_argv(), "");


      //Get reference to the RootPOA.
      CORBA::Object_var obj = orb->resolve_initial_references( "RootPOA" );
      PortableServer::POA_var poa = PortableServer::POA::_narrow( obj.in() );

      // Activate the POAManager.
      PortableServer::POAManager_var mgr = poa->the_POAManager();
      mgr->activate();

      OpenDDS::DCPS::Discovery_rch disc;

      if (use_rtps) {
        OpenDDS::RTPS::RtpsDiscovery_rch rtpsDisc(OpenDDS::DCPS::make_rch<OpenDDS::RTPS::RtpsDiscovery>("TestRtpsDiscovery"));
        rtpsDisc->resend_period(OpenDDS::DCPS::TimeDuration(1));
        rtpsDisc->sedp_multicast(false);
        disc = rtpsDisc;
      } else {
        TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_REPO);

        CORBA::Object_var tmp =
          orb->string_to_object (ACE_TEXT_ALWAYS_CHAR(ior));
        OpenDDS::DCPS::DCPSInfo_var info =
          OpenDDS::DCPS::DCPSInfo::_narrow (tmp.in ());
        OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::InfoRepoDiscovery> ird(
          OpenDDS::DCPS::make_rch<OpenDDS::DCPS::InfoRepoDiscovery>("TestInfoRepoDiscovery", info));
        disc = OpenDDS::DCPS::static_rchandle_cast<OpenDDS::DCPS::Discovery>(ird);
        ird->set_ORB(orb);
      }

      if (disc.is_nil())
        {
          ACE_ERROR_RETURN((LM_DEBUG,
                            "(%P|%t) ERROR: Nil OpenDDS::DCPS::Discovery reference <%s>\n",
                            ior),
                           1);
        }

/*
      typedef void (ACE_Log_Msg::*PTMF)(u_long);
      PTMF flagop = &ACE_Log_Msg::set_flags;
      (ACE_LOG_MSG->*flagop)(ACE_Log_Msg::VERBOSE_LITE);
      OpenDDS::DCPS::DCPS_debug_level = 4;
      OpenDDS::DCPS::Transport_debug_level = 4;
*/
      if (!pubsub(disc, orb))
        {
          return 1;
        }

      disc.reset();
      obj = 0;
      poa = 0;
      mgr = 0;

      // clean up the orb
      orb->destroy ();

      TheServiceParticipant->shutdown ();
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in publisher.cpp:");
      return 1;
    }
  catch (const std::exception& ex)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: std::exception caught in main.cpp: %C\n"),
                  ex.what()));
      return 1;
    }

  return failed;
}

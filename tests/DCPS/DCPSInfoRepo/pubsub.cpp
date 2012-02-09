#include "DCPSDataWriterI.h"
#include "DCPSDataReaderI.h"
#include "DCPSSubscriberI.h"

#include "dds/DdsDcpsInfoC.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/MonitorFactory.h"

#ifndef DDS_HAS_MINIMUM_BIT
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/RTPS/RtpsInfo.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/Argv_Type_Converter.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

const ACE_TCHAR* ior = ACE_TEXT("file://dcps_ir.ior");
bool qos_tests = false;
bool use_rtps = false;
bool failed = false;

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
  // Indicates sucessful parsing of the command line
  return 0;
}

bool pubsub(OpenDDS::DCPS::DCPSInfo_var info, CORBA::ORB_var orb, PortableServer::POA_var poa)
{
#ifndef DDS_HAS_MINIMUM_BIT
  OpenDDS::RTPS::RtpsInfo* rtpsInfo = 0;
  PortableServer::ServantBase_var rtpsInfoVar;
  if (use_rtps) {
    rtpsInfoVar = poa->reference_to_servant(info.in());
    rtpsInfo = dynamic_cast<OpenDDS::RTPS::RtpsInfo*>(rtpsInfoVar.in());
  }
#endif

  CORBA::Long domain = 9;

  OpenDDS::DCPS::RepoId pubPartId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId pubTopicId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId pubId = OpenDDS::DCPS::GUID_UNKNOWN;
  TAO_DDS_DCPSDataWriter_i* dwImpl = new TAO_DDS_DCPSDataWriter_i;
  PortableServer::ServantBase_var safe_servant = dwImpl;
  OpenDDS::DCPS::DataWriterRemote_var dw;

  ::DDS::DomainParticipantQos_var partQos = new ::DDS::DomainParticipantQos;
  OpenDDS::DCPS::AddDomainStatus value = info->add_domain_participant(domain, partQos.in());
  pubPartId = value.id;
  if (OpenDDS::DCPS::GUID_UNKNOWN == pubPartId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_domain_participant failed!\n") ));
    }

#ifndef DDS_HAS_MINIMUM_BIT
  DDS::Subscriber_var sub;
  DDS::Subscriber_var sub2;
  if (use_rtps) {
    sub = new TAO_DDS_DCPSSubscriber_i;
    rtpsInfo->init_bit(pubPartId, domain, sub);
  }
#endif

  // add a topic
  const char* tname = "MYtopic";
  const char* dname = "MYdataname";
  ::DDS::TopicQos_var topicQos = new ::DDS::TopicQos;
  OpenDDS::DCPS::TopicStatus topicStatus = info->assert_topic(pubTopicId,
                                                       domain,
                                                       pubPartId,
                                                       tname,
                                                       dname,
                                                       topicQos.in(),
                                                       false);

  if (topicStatus != OpenDDS::DCPS::CREATED)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Topic creation failed and returned %d"), topicStatus));
    }

  // Add publication
  PortableServer::ObjectId_var oid = poa->activate_object( dwImpl );
  CORBA::Object_var obj = poa->id_to_reference( oid.in() );
  dw = OpenDDS::DCPS::DataWriterRemote::_narrow(obj.in());
  if (CORBA::is_nil (dw.in ()))
    {
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "Nil OpenDDS::DCPS::DataWriterRemote reference\n"),
                        false);
    }

  if (dwImpl->numReceived() > 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should not receive anything after ")
        ACE_TEXT("it is created since there are no readers, received %d messages\n"),
        dwImpl->numReceived()));
    }

  ::DDS::DataWriterQos_var dwQos = new ::DDS::DataWriterQos;
  *dwQos = TheServiceParticipant->initial_DataWriterQos();
  dwQos->reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  OpenDDS::DCPS::TransportLocatorSeq tii;
  tii.length(1);
  tii[0].transport_type = "fake transport for test";

  ::DDS::PublisherQos_var pQos = new ::DDS::PublisherQos;

  pubId = info->add_publication(domain,
                                pubPartId,
                                pubTopicId,
                                dw.in(),
                                dwQos.in(),
                                tii,
                                pQos.in());
  if (OpenDDS::DCPS::GUID_UNKNOWN == pubId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_publication failed!\n") ));
    }

  if (dwImpl->numReceived() > 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should not receive anything after pub added ")
        ACE_TEXT("since there are no readers, received %d messages\n"),
        dwImpl->numReceived()));
    }

  // add an inconsistent topic
  OpenDDS::DCPS::RepoId topicId2;
  const char* tnameIncompatible = "MYtopic";
  const char* dnameIncompatible = "MYnewdataname";
  ::DDS::TopicQos_var topicQosIncompatible = new ::DDS::TopicQos;
  topicStatus = info->assert_topic(topicId2,
                                                        domain,
                                                        pubPartId,
                                                        tnameIncompatible,
                                                        dnameIncompatible,
                                                        topicQosIncompatible.in(),
                                                        false);

  if (topicStatus != OpenDDS::DCPS::CONFLICTING_TYPENAME)
    {
      failed = true;
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: Inconsistent topic creation did not fail with ")
                 ACE_TEXT("CONFLICTING_TYPENAME and returned %d"), topicStatus));
    }

  if (dwImpl->numReceived() > 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should not receive anything after ")
        ACE_TEXT("attempting to create an inconsistent topic, received %d messages\n"),
        dwImpl->numReceived()));
    }

  OpenDDS::DCPS::RepoId subPartId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId subTopicId = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::RepoId subId = OpenDDS::DCPS::GUID_UNKNOWN;
  TAO_DDS_DCPSDataReader_i* drImpl = new TAO_DDS_DCPSDataReader_i;
  PortableServer::ServantBase_var safe_servant2 = drImpl;
#ifndef DDS_HAS_MINIMUM_BIT
  drImpl->info_ = rtpsInfo;
#endif
  OpenDDS::DCPS::DataReaderRemote_var dr;

  value = info->add_domain_participant(domain, partQos.in());
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

#ifndef DDS_HAS_MINIMUM_BIT
  if (use_rtps) {
    sub2 = new TAO_DDS_DCPSSubscriber_i;
    rtpsInfo->init_bit(subPartId, domain, sub2);
  }
#endif

  topicStatus = info->assert_topic(subTopicId,
                                   domain,
                                   subPartId,
                                   tname,
                                   dname,
                                   topicQos.in(),
                                   false);

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

  if (dwImpl->numReceived() > 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should not receive anything after ")
        ACE_TEXT("the same topic is created for another domain participant, received %d messages\n"),
        dwImpl->numReceived()));
    }

  drImpl->domainId_ = domain;
  drImpl->participantId_ = subPartId;

  // Add subscription
  oid = poa->activate_object( drImpl );
  obj = poa->id_to_reference( oid.in() );
  dr = OpenDDS::DCPS::DataReaderRemote::_narrow(obj.in());
  if (CORBA::is_nil (dr.in ()))
    {
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "Nil OpenDDS::DCPS::DataReaderRemote reference\n"),
                        false);
    }

  ::DDS::DataReaderQos_var drQos = new ::DDS::DataReaderQos;
  *drQos = TheServiceParticipant->initial_DataReaderQos();
  drQos->reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;

  ::DDS::SubscriberQos_var subQos = new ::DDS::SubscriberQos;

  subId = info->add_subscription(domain,
                                 subPartId,
                                 subTopicId,
                                 dr.in(),
                                 drQos.in(),
                                 tii,
                                 subQos.in(),
                                 "", DDS::StringSeq());
  if( OpenDDS::DCPS::GUID_UNKNOWN == subId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_subscription failed!\n") ));
    }

  ACE_Time_Value run_time = ACE_Time_Value(3,0);
  orb->run(run_time);

  TAO_DDS_DCPSDataWriter_i::Called wCalled;
  TAO_DDS_DCPSDataReader_i::Called rCalled;
  if (dwImpl->numReceived() == 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should have had add_association ")
        ACE_TEXT("called, but nothing was received\n")));
    }
  else
    {
      if ((wCalled = dwImpl->next()) != TAO_DDS_DCPSDataWriter_i::ADD_ASSOC)
        {
          failed = true;
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should have had %d ")
            ACE_TEXT(", but instead received %d\n"),
            TAO_DDS_DCPSDataWriter_i::ADD_ASSOC,
            wCalled));
        }
      if (use_rtps)
        {
          if (dwImpl->numReceived() == 0)
            {
              failed = true;
              ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should have had association_complete called\n")));
            }
           else if ((wCalled = dwImpl->next()) != TAO_DDS_DCPSDataWriter_i::ASSOC_COMPLETE)
            {
              failed = true;
              ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should have had association_complete ")
                ACE_TEXT("called, but instead received %d\n"),
                wCalled));
            }
        }
      else if (dwImpl->numReceived() > 0)
        {
          failed = true;
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer was not expecting any more calls,")
            ACE_TEXT("but received %d more\n"),
            dwImpl->numReceived()));
        }
    }

  if (drImpl->numReceived() == 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Reader should have had add_association ")
        ACE_TEXT("called, but nothing was received\n")));
    }
  else
    {
      if ((rCalled = drImpl->next()) != TAO_DDS_DCPSDataReader_i::ADD_ASSOC)
        {
          failed = true;
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Reader should have had %d, but instead received %d\n"),
            TAO_DDS_DCPSDataReader_i::ADD_ASSOC,
            rCalled));
        }
    }


  // incompatible QOS
  OpenDDS::DCPS::RepoId pubIncQosId = OpenDDS::DCPS::GUID_UNKNOWN;
  TAO_DDS_DCPSDataWriter_i* dwIncQosImpl = new TAO_DDS_DCPSDataWriter_i;
  PortableServer::ServantBase_var safe_servant3 = dwIncQosImpl;
  OpenDDS::DCPS::DataWriterRemote_var dwIncQos;

  // Add publication
  oid = poa->activate_object( dwIncQosImpl );
  obj = poa->id_to_reference( oid.in() );
  dwIncQos = OpenDDS::DCPS::DataWriterRemote::_narrow(obj.in());
  if (CORBA::is_nil (dwIncQos.in ()))
    {
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "Nil OpenDDS::DCPS::DataWriterRemote reference\n"),
                        false);
    }

  if (dwIncQosImpl->numReceived() > 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should not receive anything after ")
        ACE_TEXT("it is created since there are no readers, received %d messages\n"),
        dwIncQosImpl->numReceived()));
    }

  ::DDS::DataWriterQos_var dwIncQosQos = new ::DDS::DataWriterQos;
  *dwIncQosQos = TheServiceParticipant->initial_DataWriterQos();
  dwIncQosQos->reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;

  pubIncQosId = info->add_publication(domain,
                                pubPartId,
                                pubTopicId,
                                dwIncQos.in(),
                                dwIncQosQos.in(),
                                tii,
                                pQos.in());
  if (OpenDDS::DCPS::GUID_UNKNOWN == pubIncQosId)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: add_publication failed!\n") ));
    }

  run_time = ACE_Time_Value(6,0);
  orb->run(run_time);

  if (dwIncQosImpl->numReceived() == 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should have had update_incompatible_qos ")
        ACE_TEXT("called, but nothing was received\n")));
    }
  else
    {
      if ((wCalled = dwIncQosImpl->next()) != TAO_DDS_DCPSDataWriter_i::UPDATE_INCOMP_QOS)
        {
          failed = true;
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer should have had %d ")
            ACE_TEXT(", but instead received %d\n"),
            TAO_DDS_DCPSDataWriter_i::UPDATE_INCOMP_QOS,
            wCalled));
        }
      if (dwIncQosImpl->numReceived() > 0)
        {
          failed = true;
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Writer was not expecting any more calls,")
            ACE_TEXT("but received %d more\n"),
            dwIncQosImpl->numReceived()));
        }
    }

  if (drImpl->numReceived() == 0)
    {
      failed = true;
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Reader should have had add_association ")
        ACE_TEXT("called, but nothing was received\n")));
    }
  else
    {
      if ((rCalled = drImpl->next()) != TAO_DDS_DCPSDataReader_i::UPDATE_INCOMP_QOS)
        {
          failed = true;
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Reader should have had %d, but instead received %d\n"),
            TAO_DDS_DCPSDataReader_i::UPDATE_INCOMP_QOS,
            rCalled));
        }
    }

  info->remove_publication(domain, pubPartId, pubIncQosId);
  info->remove_subscription(domain, subPartId, subId);
  info->remove_publication(domain, pubPartId, pubId);
  info->remove_topic(domain, pubPartId, pubTopicId);
  info->remove_topic(domain, subPartId, subTopicId);
  info->remove_domain_participant(domain, subPartId);
  info->remove_domain_participant(domain, pubPartId);

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

      TheServiceParticipant->set_ORB(orb);

      //Get reference to the RootPOA.
      CORBA::Object_var obj = orb->resolve_initial_references( "RootPOA" );
      PortableServer::POA_var poa = PortableServer::POA::_narrow( obj.in() );

      // Activate the POAManager.
      PortableServer::POAManager_var mgr = poa->the_POAManager();
      mgr->activate();

      OpenDDS::DCPS::DCPSInfo_var info;
#ifndef DDS_HAS_MINIMUM_BIT
      OpenDDS::RTPS::RtpsDiscovery_rch disc;

      if (use_rtps) {
        disc = new OpenDDS::RTPS::RtpsDiscovery("TestRtpsDiscovery");
        disc->resend_period(ACE_Time_Value(1));
        disc->sedp_multicast(false);
        info = disc->get_dcps_info();

      } else {
#endif
        CORBA::Object_var tmp =
          orb->string_to_object (ACE_TEXT_ALWAYS_CHAR(ior));
        info = OpenDDS::DCPS::DCPSInfo::_narrow (tmp.in ());
#ifndef DDS_HAS_MINIMUM_BIT
      }
#endif

      if (CORBA::is_nil (info.in ()))
        {
          ACE_ERROR_RETURN ((LM_DEBUG,
                             "Nil OpenDDS::DCPS::DCPSInfo reference <%s>\n",
                             ior),
                            1);
        }

      if (!pubsub(info, orb, poa))
        {
          return 1;
        }

#ifndef DDS_HAS_MINIMUM_BIT
      disc = 0;
#endif
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

  return failed;
}

#include "dds/DdsDcpsInfoC.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "DCPSDataWriterI.h"

#include "ace/Arg_Shifter.h"
#include "ace/Argv_Type_Converter.h"

const ACE_TCHAR *ior = ACE_TEXT("file://dcps_ir.ior");
bool ignore_entities = false;
bool qos_tests = false;

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0)
    {
      ior = currentArg;
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-i")) == 0)
    {
      ignore_entities = true;
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-q")) == 0)
    {
      qos_tests = true;
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                          "usage:  %s "
                          "-k <ior> "
                          "\n"
                          "        -i (ignore tests)\n"
                          "        -q (incompatible qos test)\n"
                          "        -? (usage message)",
                          argv [0]),
                        -1);
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  if (parse_args (argc, argv) != 0)
    return 1;

  try
    {
      ACE_Argv_Type_Converter converter (argc, argv);

      CORBA::ORB_var orb =
        CORBA::ORB_init (converter.get_argc(), converter.get_ASCII_argv() , "");

      //Get reference to the RootPOA.
      CORBA::Object_var obj = orb->resolve_initial_references( "RootPOA" );
      PortableServer::POA_var poa = PortableServer::POA::_narrow( obj.in() );

      // Activate the POAManager.
      PortableServer::POAManager_var mgr = poa->the_POAManager();
      mgr->activate();

      CORBA::Object_var tmp =
        orb->string_to_object (ACE_TEXT_ALWAYS_CHAR(ior));

      OpenDDS::DCPS::DCPSInfo_var info =
        OpenDDS::DCPS::DCPSInfo::_narrow (tmp.in ());

      if (CORBA::is_nil (info.in ()))
        {
          ACE_ERROR_RETURN ((LM_DEBUG,
                             "Nil OpenDDS::DCPS::DCPSInfo reference <%s>\n",
                             ior),
                            1);
        }


      // check adding a participant
      ::DDS::DomainParticipantQos_var dpQos = new ::DDS::DomainParticipantQos;
      CORBA::Long domainId = 911;

      OpenDDS::DCPS::AddDomainStatus value = info->add_domain_participant(domainId, dpQos.in());
      OpenDDS::DCPS::RepoId dpId = value.id;
      if (OpenDDS::DCPS::GUID_UNKNOWN == dpId)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("add_domain_participant failed!\n") ));
        }

      // add a topic
      OpenDDS::DCPS::RepoId topicId;
      const char* tname = "MYtopic";
      const char* dname = "MYdataname";
      ::DDS::TopicQos_var topicQos = new ::DDS::TopicQos;
      OpenDDS::DCPS::TopicStatus topicStatus = info->assert_topic(topicId,
                                                           domainId,
                                                           dpId,
                                                           tname,
                                                           dname,
                                                           topicQos.in());

      if (topicStatus != OpenDDS::DCPS::CREATED)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("Topic creation failed and returned %d"), topicStatus));
        }

      // Add publication
      TAO_DDS_DCPSDataWriter_i dwi;
      PortableServer::ObjectId_var oid = poa->activate_object( &dwi );
      obj = poa->id_to_reference( oid.in() );
      OpenDDS::DCPS::DataWriterRemote_var dw = OpenDDS::DCPS::DataWriterRemote::_narrow(obj.in());
      if (CORBA::is_nil (dw.in ()))
        {
          ACE_ERROR_RETURN ((LM_DEBUG,
                             "Nil OpenDDS::DCPS::DataWriterRemote reference\n"),
                            1);
        }

      ::DDS::DataWriterQos_var dwq = new ::DDS::DataWriterQos;
      dwq->reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      OpenDDS::DCPS::TransportLocatorSeq_var tii = new OpenDDS::DCPS::TransportLocatorSeq;
      ::DDS::PublisherQos_var pQos = new ::DDS::PublisherQos;

      OpenDDS::DCPS::RepoId pubId = info->add_publication(domainId,
                                                          dpId,
                                                          topicId,
                                                          dw.in(),
                                                          dwq.in(),
                                                          tii.in(),
                                                          pQos.in());
      if (OpenDDS::DCPS::GUID_UNKNOWN == pubId)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("add_publication failed!\n") ));
        }

      // add an inconsistent topic
      OpenDDS::DCPS::RepoId topicId2;
      const char* tname2 = "MYtopic";
      const char* dname2 = "MYnewdataname";
      ::DDS::TopicQos_var topicQos2 = new ::DDS::TopicQos;
      OpenDDS::DCPS::TopicStatus topicStatus2 = info->assert_topic(topicId2,
                                                            domainId,
                                                            dpId,
                                                            tname2,
                                                            dname2,
                                                            topicQos2.in());

      if (topicStatus2 != OpenDDS::DCPS::CONFLICTING_TYPENAME)
        {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Inconsistent topic creation did not fail with ")
                     ACE_TEXT("CONFLICTING_TYPENAME and returned %d"), topicStatus));
        }



      if (ignore_entities)
        {
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("Ignoring all entities with 1 and 2\n") ));

          OpenDDS::DCPS::RepoIdBuilder builder;

          builder.entityKey(1);
          OpenDDS::DCPS::RepoId repoId_1(builder);

          info->ignore_domain_participant(domainId, dpId, repoId_1);
          info->ignore_topic(domainId, dpId, repoId_1);
          info->ignore_publication(domainId, dpId, repoId_1);
          info->ignore_subscription(domainId, dpId, repoId_1);

          builder.entityKey(2);
          OpenDDS::DCPS::RepoId repoId_2(builder);

          info->ignore_domain_participant(domainId, dpId, repoId_2);
          info->ignore_topic(domainId, dpId, repoId_2);
          info->ignore_publication(domainId, dpId, repoId_2);
          info->ignore_subscription(domainId, dpId, repoId_2);
        }


      // Set up the incompatible qos test
      OpenDDS::DCPS::RepoId dpIdAlmost = OpenDDS::DCPS::GUID_UNKNOWN;
      OpenDDS::DCPS::RepoId topicIdAlmost;
      TAO_DDS_DCPSDataWriter_i* dwiAlmost = new TAO_DDS_DCPSDataWriter_i;
      PortableServer::ServantBase_var safe_servant = dwiAlmost;
      OpenDDS::DCPS::DataWriterRemote_var dwAlmost;
      ::DDS::DataWriterQos_var dwqAlmost = 0;
      OpenDDS::DCPS::RepoId pubIdAlmost = OpenDDS::DCPS::GUID_UNKNOWN;

      if (qos_tests)
      {

        ::OpenDDS::DCPS::AddDomainStatus value = info->add_domain_participant(domainId, dpQos.in());
        dpIdAlmost = value.id;
        if( OpenDDS::DCPS::GUID_UNKNOWN == dpIdAlmost)
          {
            ACE_ERROR((LM_ERROR, ACE_TEXT("add_domain_participant for qos test failed!\n") ));
          }

        // add a topic
        topicStatus = info->assert_topic(topicIdAlmost,
                                         domainId,
                                         dpIdAlmost,
                                         tname,
                                         dname,
                                         topicQos.in());

        if (topicStatus != OpenDDS::DCPS::CREATED)
          {
            ACE_ERROR((LM_ERROR,
                      ACE_TEXT("Topic creation for qos test failed and returned %d"),
                      topicStatus));
          }

        // Add publication
        oid = poa->activate_object( dwiAlmost );
        obj = poa->id_to_reference( oid.in() );
        dwAlmost = OpenDDS::DCPS::DataWriterRemote::_narrow(obj.in());
        if (CORBA::is_nil (dwAlmost.in ()))
          {
            ACE_ERROR_RETURN ((LM_DEBUG,
                              "Nil OpenDDS::DCPS::DataWriterRemote reference in qos test\n"),
                              1);
          }

        dwqAlmost = new ::DDS::DataWriterQos;
        dwqAlmost->reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;

        pubIdAlmost = info->add_publication(domainId,
                                            dpIdAlmost,
                                            topicIdAlmost,
                                            dwAlmost.in(),
                                            dwqAlmost.in(),
                                            tii.in(),
                                            pQos.in());
        if( OpenDDS::DCPS::GUID_UNKNOWN == pubId)
          {
            ACE_ERROR((LM_ERROR, ACE_TEXT("add_publication for qos test failed!\n") ));
          }
      }




      // run the orb
      ACE_Time_Value run_time = ACE_Time_Value(15,0);
      orb->run(run_time);

      if (ignore_entities)
        {
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("Ignoring all entities with 3\n") ));

          OpenDDS::DCPS::RepoIdBuilder builder;

          builder.entityKey(3);
          OpenDDS::DCPS::RepoId repoId_3(builder);

          info->ignore_domain_participant(domainId, dpId, repoId_3);
          info->ignore_topic(domainId, dpId, repoId_3);
          info->ignore_publication(domainId, dpId, repoId_3);
          info->ignore_subscription(domainId, dpId, repoId_3);

          run_time = ACE_Time_Value(15,0);
          orb->run(run_time);
        }

      if (qos_tests)
        {
          // remove all the qos test entities
          info->remove_publication(domainId, dpIdAlmost, pubIdAlmost);

          info->remove_topic(domainId, dpIdAlmost, topicIdAlmost);

          info->remove_domain_participant(domainId, dpIdAlmost);
        }


      // remove all the entities
      info->remove_publication(domainId, dpId, pubId);

      info->remove_topic(domainId, dpId, topicId);

      info->remove_domain_participant(domainId, dpId);
      // clean up the orb
      orb->destroy ();
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in publisher.cpp:");
      return 1;
    }

  return 0;
}

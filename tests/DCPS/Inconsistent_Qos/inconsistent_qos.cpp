// -*- C++ -*-
// ============================================================================
/**
 *  @file   inconsistent_topic.cpp
 *
 */
// ============================================================================

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/QOS_XML_Handler/XML_File_Intf.h>

#include "dds/DCPS/StaticIncludes.h"
#include "MessengerTypeSupportImpl.h"

#include <ace/streams.h>
#include "ace/Get_Opt.h"

#include <memory>

const ACE_TCHAR * xml_file_ = 0;

int
parse_args(int argc, ACE_TCHAR *argv[])
{
  //
  // Command-line Options:
  //
  //    -x <qos xml file>
  //

  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("x:"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'x':
      xml_file_ = get_opts.opt_arg();
      std::cout << "parse_args: xml_file = " << xml_file_ << std::endl;
      break;

    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C -x qos_xml_file\n"), argv[0]),
                       -1);
    }
  }

  return 0;
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[]){
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      int error;
      if ((error = parse_args(argc, argv)) != 0) {
        return error;
      }


      DDS::DomainParticipant_var participant =
        dpf->create_participant(11,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }
      else
      {
        ACE_DEBUG ((LM_DEBUG, "Created participant 1 with instance handle %d\n",
                    participant->get_instance_handle ()));
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                        -1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                        -1);
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                        -1);
      }


      // Create DataWriter
      // Use the qos xml file for that
      OpenDDS::DCPS::QOS_XML_File_Handler *xml_file_loader =
        new OpenDDS::DCPS::QOS_XML_File_Handler ();

      try {
        xml_file_loader->add_search_path (
          ACE_TEXT("DDS_ROOT"),
          ACE_TEXT("/docs/schema/"));
      } catch (const std::exception&) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                        -1);
      }

      DDS::ReturnCode_t const retcode = xml_file_loader->init (xml_file_);
      if (retcode != ::DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT("Error while initializing QOS Handler <%d>\n"),
                      retcode));
          delete xml_file_loader;
          return 1;
        }

      ::DDS::DataWriterQos dw_qos;

      pub->get_default_datawriter_qos (dw_qos);
      xml_file_loader->get_datawriter_qos (dw_qos,
                                           ACE_TEXT("InconsistentQos"),
                                           ACE_TEXT(""));
      delete xml_file_loader;

      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                              dw_qos,
                              DDS::DataWriterListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode1 = participant->delete_topic (topic.in ());
      if (retcode1 != DDS::RETCODE_PRECONDITION_NOT_MET) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should not be able to delete topic, still referenced by datawriter!\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode2 = pub->delete_datawriter (dw.in ());
      if (retcode2 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete datawriter\n")),
                        -1);
      }
      dw = DDS::DataWriter::_nil ();


      DDS::ReturnCode_t retcode3 = participant->delete_publisher (pub.in ());
      if (retcode3 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete publisher\n")),
                        -1);
      }
      pub = DDS::Publisher::_nil ();

      dpf->delete_participant(participant.in ());
      participant = DDS::DomainParticipant::_nil ();
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
  {
    cerr << "dp: Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }
  return 0;
}

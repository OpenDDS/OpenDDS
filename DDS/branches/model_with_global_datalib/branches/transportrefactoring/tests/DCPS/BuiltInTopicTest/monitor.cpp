// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"

#include <ace/streams.h>
#include "ace/Get_Opt.h"

int delay_before_read_sec = 0;
unsigned int num_parts = 4;
unsigned int num_topics = 2;
unsigned int num_subs = 1;
unsigned int num_pubs = 1;
const char* topic_name = "Movie Discussion List";
const char* topic_type_name = "Messenger";

int
parse_args (int argc, char *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "l:d:t:s:p:");
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 'l':
      delay_before_read_sec = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case 'd':
      num_parts = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case 't':
      num_topics = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case 's':
      num_subs = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case 'p':
      num_pubs = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        "usage:  %s "
        "-l <delay_before_read_sec> "
        "-d <num of domain participant> "
        "-t <num of topics> "
        "-s <num of subs> "
        "-p <num of pubs> "
        "\n",
        argv [0]),
        -1);
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      participant = dpf->create_participant(411,
                                            PARTICIPANT_QOS_DEFAULT,
                                            DDS::DomainParticipantListener::_nil());
      if (CORBA::is_nil (participant.in ())) {
        ACE_ERROR((LM_ERROR, "(%P|%t)create_participant failed.\n"));
        return 1 ;
      }
      // give time for BIT datareader/datawriter fully association.
      ACE_OS::sleep (2);

      if (delay_before_read_sec > 0)
        ACE_OS::sleep (delay_before_read_sec);
 
      ::DDS::Subscriber_var bit_subscriber
        = participant->get_builtin_subscriber () ;

      ::DDS::DataReader_var reader
        = bit_subscriber->lookup_datareader (OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC) ;

      ::DDS::ParticipantBuiltinTopicDataDataReader_var part_reader
        = ::DDS::ParticipantBuiltinTopicDataDataReader::_narrow (reader.in ());
      if (CORBA::is_nil (part_reader.in ()))
      {
        ACE_ERROR((LM_ERROR, "(%P|%t)failed to get BUILT_IN_PARTICIPANT_TOPIC datareader.\n"));
        return 1;
      }

      reader = bit_subscriber->lookup_datareader (OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC) ;
      ::DDS::TopicBuiltinTopicDataDataReader_var topic_reader
        = ::DDS::TopicBuiltinTopicDataDataReader::_narrow (reader.in ());
      if (CORBA::is_nil (topic_reader.in ()))
      {
        ACE_ERROR((LM_ERROR, "(%P|%t)failed to get BUILT_IN_TOPIC_TOPIC datareader.\n"));
        return 1;
      }

      reader = bit_subscriber->lookup_datareader (OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC) ;
      ::DDS::SubscriptionBuiltinTopicDataDataReader_var sub_reader
        = ::DDS::SubscriptionBuiltinTopicDataDataReader::_narrow (reader.in ());
      if (CORBA::is_nil (sub_reader.in ()))
      {
        ACE_ERROR((LM_ERROR, "(%P|%t)failed to get BUILT_IN_SUBSCRIPTION_TOPIC datareader.\n"));
        return 1;
      }

      reader = bit_subscriber->lookup_datareader (OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC) ;
      ::DDS::PublicationBuiltinTopicDataDataReader_var pub_reader
        = ::DDS::PublicationBuiltinTopicDataDataReader::_narrow (reader.in ());
      if (CORBA::is_nil (pub_reader.in ()))
      {
        ACE_ERROR((LM_ERROR, "(%P|%t)failed to get BUILT_IN_PUBLICATION_TOPIC datareader.\n"));
        return 1;
      }

      ::DDS::SampleInfoSeq partinfos(10);
      ::DDS::ParticipantBuiltinTopicDataSeq partdata(10);
      DDS::ReturnCode_t ret = part_reader->read (partdata,
                                                 partinfos,
                                                 10,
                                                 ::DDS::ANY_SAMPLE_STATE,
                                                 ::DDS::ANY_VIEW_STATE,
                                                 ::DDS::ANY_INSTANCE_STATE);
 
      if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) failed to read BIT participant data. "),
            1);
        }

      CORBA::ULong len = partdata.length ();

      if (len != num_parts)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          "(%P|%t) read %d BIT part data, expected %d parts. ", len, num_parts),
          1);
      }

      for (CORBA::ULong i = 0; i < len; ++i)
      {
        ACE_DEBUG((LM_DEBUG, "(%P|%t)Participant: key = %d, %d, %d \n",
          partdata[i].key[0], partdata[i].key[1], partdata[i].key[2]));
      }

      ::DDS::SampleInfoSeq topicinfos(10);
      ::DDS::TopicBuiltinTopicDataSeq topicdata(10);
      ret = topic_reader->read (topicdata,
        topicinfos,
        10,
        ::DDS::ANY_SAMPLE_STATE,
        ::DDS::ANY_VIEW_STATE,
        ::DDS::ANY_INSTANCE_STATE);

      if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) failed to read BIT topic data. "),
            1);
        }

      len = topicdata.length ();

      if (len != num_topics)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          "(%P|%t) read %d BIT topic data, expected %d topics. ", len, num_topics),
          1);
      }

      for (CORBA::ULong i = 0; i < len; ++i)
      {
        if (ACE_OS::strcmp (topicdata[i].name.in (), topic_name) != 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) got topic name \"%s\", expected topic name \"%s\"\n",
            topicdata[i].name.in (), topic_name),
            1);
        }
        if (ACE_OS::strcmp (topicdata[i].type_name.in (), topic_type_name) != 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) got topic type name \"%s\", expected topic type name \"%s\"\n",
            topicdata[i].type_name.in (), topic_type_name),
            1);
        }

        ACE_DEBUG((LM_DEBUG, "(%P|%t)Topic: key = %d, %d, %d, name = %s, "
          "type_name=%s \n",
          topicdata[i].key[0], topicdata[i].key[1], topicdata[i].key[2],
          topicdata[i].name.in (), topicdata[i].type_name.in ()));
      }

      ::DDS::SampleInfoSeq pubinfos(10);
      ::DDS::PublicationBuiltinTopicDataSeq pubdata(10);
      ret = pub_reader->read (pubdata,
        pubinfos,
        10,
        ::DDS::ANY_SAMPLE_STATE,
        ::DDS::ANY_VIEW_STATE,
        ::DDS::ANY_INSTANCE_STATE);

      if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) failed to read BIT publication data. "),
            1);
        }

      len = pubdata.length ();
 
      if (len != num_pubs)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          "(%P|%t) read %d BIT pub data, expected %d pubs. ", len, num_pubs),
          1);
      }
 
      for (CORBA::ULong i = 0; i < len; ++i)
      {
        if (ACE_OS::strcmp (pubdata[i].topic_name.in (), topic_name) != 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) got datawriter topic name \"%s\", expected topic name \"%s\"\n",
            pubdata[i].topic_name.in (), topic_name),
            1);
        }
        if (ACE_OS::strcmp (pubdata[i].type_name.in (), topic_type_name) != 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) got datawriter topic type name \"%s\", expected topic type name \"%s\"\n",
            pubdata[i].type_name.in (), topic_type_name),
            1);
        }
        ACE_DEBUG((LM_DEBUG, "(%P|%t)DataWriter: key = %d, %d, %d \n",
          pubdata[i].key[0], pubdata[i].key[1], pubdata[i].key[2]));
      }

      ::DDS::SampleInfoSeq subinfos(10);
      ::DDS::SubscriptionBuiltinTopicDataSeq subdata(10);
      ret = sub_reader->read (subdata,
        subinfos,
        10,
        ::DDS::ANY_SAMPLE_STATE,
        ::DDS::ANY_VIEW_STATE,
        ::DDS::ANY_INSTANCE_STATE);
 
      if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) failed to read BIT subsciption data. "),
            1);
        }

      len = subdata.length ();

      if (len != num_subs)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          "(%P|%t) read %d BIT sub data, expected %d subs. ", len, num_subs),
          1);
      }

      for (CORBA::ULong i = 0; i < len; ++i)
      {
        if (ACE_OS::strcmp (subdata[i].topic_name.in (), topic_name) != 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) got datareader topic name \"%s\", expected topic name \"%s\"\n",
            subdata[i].topic_name.in (), topic_name),
            1);
        }
        if (ACE_OS::strcmp (subdata[i].type_name.in (), topic_type_name) != 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            "(%P|%t) got datareader topic type name \"%s\", expected topic type name \"%s\"\n",
            subdata[i].type_name.in (), topic_type_name),
            1);
        }
 
        ACE_DEBUG((LM_DEBUG, "(%P|%t)DataReader: key = %d, %d, %d \n",
          subdata[i].key[0], subdata[i].key[1], subdata[i].key[2]));
      }


      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}

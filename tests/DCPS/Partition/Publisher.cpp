
#include "TestTypeSupportImpl.h"
#include "Partition_Table.h"
#include "DataWriterListener.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>

#include "dds/DCPS/StaticIncludes.h"

#include <vector>
#include <algorithm>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using std::cerr;
using std::endl;


int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }

      Test::DataTypeSupportImpl* servant = new Test::DataTypeSupportImpl();

      if (DDS::RETCODE_OK != servant->register_type(participant.in (), "")) {
        cerr << "register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos (topic_qos);

      DDS::Topic_var topic =
        participant->create_topic ("Data",
                                   type_name.in (),
                                   topic_qos,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
      {
        cerr << "create_topic failed." << endl;
        exit(1);
      }

      size_t const num_partitions =
        sizeof (Test::Offered::PartitionConfigs)
        / sizeof (Test::Offered::PartitionConfigs[0]);

      Test::PartitionConfig const * const begin =
        Test::Offered::PartitionConfigs;
      Test::PartitionConfig const * const end =
        begin + num_partitions;

      // Keep the writers around long enough for the publications and
      // subscriptions to match.
      typedef std::vector<DDS::DataWriter_var> writers_type;
      writers_type writers (num_partitions);

      for (Test::PartitionConfig const * i = begin; i != end; ++i)
      {
        DDS::PublisherQos pub_qos;
        participant->get_default_publisher_qos (pub_qos);

        // Specify partitions we're offering.
        CORBA::ULong n = 0;
        DDS::StringSeq & names = pub_qos.partition.name;
        for (char const * const * s = (*i).partitions;
             s != 0 && *s != 0;
             ++s, ++n)
        {
          CORBA::ULong const new_len = names.length () + 1;
          names.length (new_len);
          names[n] = *s;
        }

        DDS::Publisher_var pub =
          participant->create_publisher (pub_qos,
                                         DDS::PublisherListener::_nil (),
                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (pub.in ()))
        {
          cerr << "create_publisher failed." << endl;
          exit(1);
        }

        DDS::DataWriterListener_var listener (
          new Test::DataWriterListener ((*i).expected_matches));

        // Create the datawriter
        DDS::DataWriterQos dw_qos;
        pub->get_default_datawriter_qos (dw_qos);

        DDS::DataWriter_var dw =
          pub->create_datawriter(topic.in (),
                                 dw_qos,
                                 listener.in (),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (CORBA::is_nil (dw.in ()))
        {
          cerr << "create_datawriter failed." << endl;
          exit(1);
        }

        writers.push_back (dw);

        Test::DataDataWriter_var writer
          = Test::DataDataWriter::_narrow (dw.in());
        if (CORBA::is_nil (writer.in ()))
        {
          cerr << "Data Writer could not be narrowed"<< endl;
          exit(1);
        }

        Test::Data the_data;
        the_data.key = 2;
        the_data.the_data = CORBA::string_dup ("Data Wuz Here!");

        ::DDS::InstanceHandle_t const handle =
            writer->register_instance(the_data);

        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %T PUB starting to write.\n")));

        ::DDS::ReturnCode_t const ret = writer->write (the_data, handle);;
        if (ret != ::DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: PUB ")
                      ACE_TEXT ("write() returned %d.\n"),
                      ret));
        }
      }

//       // Wait for DataReaders to finish.
//       writers_type::const_iterator zend (writers.end ());
//       for (writers_type::const_iterator z (writers.begin ()); z != zend; ++z)
//       {
//         ::DDS::InstanceHandleSeq handles;
//         while (1)
//         {
//           (*z)->get_matched_subscriptions (handles);
//           if (handles.length () == 0)
//             break;
//           else
//             ACE_OS::sleep (1);
//         }
//       }
      ACE_OS::sleep (20);
      {
        // Force contents of writers vector to be destroyed now.
        writers_type tmp;
        tmp.swap (writers);
      }

      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "PUB: Exception caught in main.cpp:" << endl
           << e << endl;
      exit(1);
    }

  return 0;
}

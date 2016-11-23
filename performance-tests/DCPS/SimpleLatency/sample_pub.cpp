//========================================================
/**
 *
 *  @file sample_pub.cpp
 *
 *  @Id: sample_pub.cpp v1.1
 *
 */
//========================================================

#include "PubListener.h"
#include "DDSPerfTestTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <ace/streams.h>

#include "ace/Get_Opt.h"
#include "ace/Sched_Params.h"
#include "ace/OS_NS_unistd.h"

#include <iostream>
#include <cstdio>

using namespace DDS;
using namespace CORBA;
using namespace DDSPerfTest;

/* void set_rt() */
/*      Attempt to set the real time priority and lock memory */
void set_rt()
{
  ACE_Sched_Params params(ACE_SCHED_FIFO,
                          ACE_DEFAULT_THREAD_PRIORITY,
                          ACE_SCOPE_PROCESS);

#if defined (ACE_HAS_WTHREADS)
  params.priority(THREAD_PRIORITY_HIGHEST);
#else
  params.priority(20);
#endif

  if (-1 == ACE_OS::sched_params(params))
  {
    ACE_DEBUG ((LM_DEBUG, "WARNING: Failed to sched_params.\n"));
  }

#if (defined (MCL_CURRENT) && defined(MCL_FUTURE) && !defined(__ANDROID__))
  if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
    ACE_DEBUG ((LM_DEBUG, "WARNING:  Could not lock memory - Run with root access.\n"));
  }
#endif
}


CORBA::Long size = 4;
long total_samples = 500;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {

       // Calling TheParticipantFactoryWithArgs before user application parse command
       // line.
       DDS::DomainParticipantFactory_var dpf =
         TheParticipantFactoryWithArgs (argc, argv);

       bool useTCP = true;
       bool useZeroCopyRead = false;
       DomainId_t myDomain = 411;

#ifndef _WIN32_WCE
       std::setbuf( stdout, NULL ); /* no buffering for standard-out */
#endif

       ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("c:ut"));
       int ich;
       while ((ich = get_opts ()) != EOF) {
        switch (ich) {

          case 'c': /* c specifes number of samples */
            total_samples = ACE_OS::atoi(get_opts.opt_arg ());

            if(total_samples < 0) {
              std::cerr << "sample_pub: ERROR - bad sample number\n";
              exit(1);
            }

            break;

          case 'u': /* u specifies that UDP should be used */
            useTCP = false;
            break;

          case 't': /* t specifies that zero copy read should be used */
            useZeroCopyRead = true;
            break;

          default: /* no parameters */
          break;

        }
       }


       /* Try to set realtime scheduling class*/
       set_rt();

       /* Create participant */
       DDS::DomainParticipant_var dp =
              dpf->create_participant (myDomain,
                                       PARTICIPANT_QOS_DEFAULT,
                                       DDS::DomainParticipantListener::_nil (),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
       if (CORBA::is_nil (dp.in ()) ) {
         std::cout << argv[0] << " SAMPLE_PUB: ERROR - Create participant failed." << endl;
         exit (1);
       }

       /* Create publisher */
       DDS::Publisher_var p =
         dp->create_publisher (PUBLISHER_QOS_DEFAULT,
                               DDS::PublisherListener::_nil (),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Initialize the transports for publisher*/
       OpenDDS::DCPS::TransportConfig_rch transport =
         TheTransportRegistry->create_config("t1");
       if (useTCP) {
         transport->instances_.push_back(
           TheTransportRegistry->create_inst("tcp", "tcp"));
       } else {
         transport->instances_.push_back(
           TheTransportRegistry->create_inst("udp", "udp"));
       }

       /* Attach the transport protocol with the publishing entity */
       TheTransportRegistry->bind_config(transport, p);


       /* Create topic for datawriter */
       PubMessageTypeSupportImpl* pubmessage_dt = new PubMessageTypeSupportImpl;
       DDS::ReturnCode_t register_status = pubmessage_dt->register_type (dp.in (),
                                                                         "DDSPerfTest::PubMessage");
       if (register_status != DDS::RETCODE_OK) {
         std::cerr << "ERROR: sample_pub failed to register PubMessage with return code "
                   << register_status << std::endl;
         exit(1);
       }

       DDS::Topic_var pubmessage_topic = dp->create_topic ("pubmessage_topic", // topic name
                                                           "DDSPerfTest::PubMessage", // topic type
                                                           TOPIC_QOS_DEFAULT,
                                                           DDS::TopicListener::_nil (),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Create PubMessage datawriter */
       DDS::DataWriter_var dw = p->create_datawriter (pubmessage_topic.in (),
                                                      DATAWRITER_QOS_DEFAULT,
                                                      DDS::DataWriterListener::_nil (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
       PubMessageDataWriter_var pubmessage_writer =
         PubMessageDataWriter::_narrow (dw.in());


       /* Create the subscriber */
       DDS::Subscriber_var s =
         dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                               DDS::SubscriberListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Attach the transport protocol with the subscribing entity */
       TheTransportRegistry->bind_config(transport, s);

       /* Create topic for datareader */
       AckMessageTypeSupportImpl* ackmessage_dt = new AckMessageTypeSupportImpl;
       register_status = ackmessage_dt->register_type (dp.in (),
                                                       "DDSPerfTest::AckMessage");
       if (register_status != DDS::RETCODE_OK) {
         std::cerr << "ERROR: sample_pub failed to register AckMessage with return code "
                   << register_status << std::endl;
         exit(1);
       }

       DDS::Topic_var ackmessage_topic = dp->create_topic ("ackmessage_topic", // topic name
                                                           "DDSPerfTest::AckMessage", // topic type
                                                           TOPIC_QOS_DEFAULT,
                                                           DDS::TopicListener::_nil (),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Create the listener for datareader */
       DDS::DataReaderListener_var listener (new AckDataReaderListenerImpl (size));
       AckDataReaderListenerImpl* listener_servant =
         dynamic_cast<AckDataReaderListenerImpl*>(listener.in());


       /* Create AckMessage datareader */
       DDS::DataReader_var dr = s->create_datareader (ackmessage_topic.in (),
                                                      DATAREADER_QOS_DEFAULT,
                                                      listener.in (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       listener_servant->init(dr.in(), dw.in(), useZeroCopyRead);

       // sleep here to wait for the connections.
       ACE_OS::sleep(1);

       AckMessageDataReader_var ackmessage_reader =
         AckMessageDataReader::_narrow (dr.in());


       DDSPerfTest::PubMessage pubmessage_data;
       pubmessage_data.seqnum = 1;

       DDS::InstanceHandle_t handle =
         pubmessage_writer->register_instance(pubmessage_data);

       pubmessage_writer->write (pubmessage_data,
                                 handle);

       while (listener_servant->done() == 0) {
         ACE_OS::sleep (1);
       }

       /* Shut down domain entities */

       std::cout << "Pub: shut down" << std::endl;
       dp->delete_contained_entities ();
       dpf->delete_participant (dp.in ());
       TheServiceParticipant->shutdown ();

  } catch (const OpenDDS::DCPS::Transport::MiscProblem &) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) sample_pub() - ")
      ACE_TEXT("Transport::MiscProblem exception caught during processing.\n")
    ));
    return 1;
  } catch (const OpenDDS::DCPS::Transport::NotFound &) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) sample_pub() - ")
      ACE_TEXT("Transport::NotFound exception caught during processing.\n")
    ));
    return 1;
  }

  return 0;

}

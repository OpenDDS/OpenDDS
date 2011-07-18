//========================================================
/**
 *
 *  @file sample_pub.cpp
 *
 *  @Id: sample_pub.cpp v1.1
 *
 */
//========================================================

#include "SubListener.h"
#include "DDSPerfTestTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/tcp/TcpInst.h>
#include <dds/DCPS/transport/udp/UdpInst.h>
#include <ace/streams.h>

#include "ace/Get_Opt.h"
#include "ace/Sched_Params.h"

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
    ACE_DEBUG ((LM_DEBUG, "WARNING: %p\n", ACE_TEXT("sched_params")));
  }

#if (defined (MCL_CURRENT) && defined(MCL_FUTURE))
  if (mlockall(MCL_CURRENT || MCL_FUTURE)) {
    ACE_DEBUG ((LM_DEBUG, "WARNING: Could not lock memory - Run with root access.\n"));
  }
#endif
}



/* Global Variables */

const OpenDDS::DCPS::TransportIdType UDP_IMPL_ID = 10;
const OpenDDS::DCPS::TransportIdType TCP_IMPL_ID = 20;


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
       // Calling TheParticipantFactoryWithArgs before user application parse command
       // line.
       /* Create participant */
       DDS::DomainParticipantFactory_var dpf =
         TheParticipantFactoryWithArgs (argc, argv);

       bool useTCP = true;
       bool useZeroCopyRead = false;
       DomainId_t myDomain = 411;

#ifndef _WIN32_WCE
       std::setbuf (stdout, NULL);
#endif

       ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("ut"));

       int ich;
       while ((ich = get_opts ()) != EOF) {
        switch (ich) {
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
         cout << argv[0] << "SAMPLE_SUB: ERROR - Create participant failed." << endl;
         exit (1);
       }

       /* Create publisher */
       DDS::Publisher_var p =
         dp->create_publisher (PUBLISHER_QOS_DEFAULT,
                               DDS::PublisherListener::_nil (),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Initialize the transport for publisher*/
       OpenDDS::DCPS::TransportImpl_rch pub_tcp_impl;
       if (useTCP) {
         pub_tcp_impl
           = TheTransportFactory->create_transport_impl (TCP_IMPL_ID,
                                                         ACE_TEXT("tcp"),
                                                         ::OpenDDS::DCPS::AUTO_CONFIG);
       } else {
         pub_tcp_impl
           = TheTransportFactory->create_transport_impl (UDP_IMPL_ID,
                                                         ACE_TEXT("udp"),
                                                         OpenDDS::DCPS::DONT_AUTO_CONFIG);
         OpenDDS::DCPS::TransportInst_rch config
           = TheTransportFactory->create_configuration (UDP_IMPL_ID, ACE_TEXT("udp"));

         OpenDDS::DCPS::UdpInst* udp_config
           = static_cast <OpenDDS::DCPS::UdpInst*> (config.in ());

         ACE_TString addrStr(ACE_LOCALHOST);
         addrStr += ACE_TEXT(":12367");
         udp_config->local_address_.set(addrStr.c_str ());
         pub_tcp_impl->configure (config.in ());
       }

       /* Attach the transport protocol with the publishing entity */
       OpenDDS::DCPS::PublisherImpl* p_impl =
         dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (p.in ());
       p_impl->attach_transport (pub_tcp_impl.in ());



       /* Create topic for datawriter */
       AckMessageTypeSupportImpl* ackmessage_dt = new AckMessageTypeSupportImpl;
       ackmessage_dt->register_type (dp.in (),
                                    "DDSPerfTest::AckMessage");
       DDS::Topic_var ackmessage_topic = dp->create_topic ("ackmessage_topic", // topic name
                                                           "DDSPerfTest::AckMessage", // topic type
                                                           TOPIC_QOS_DEFAULT,
                                                           DDS::TopicListener::_nil (),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Create PubMessage data writer */
       DDS::DataWriter_var dw = p->create_datawriter (ackmessage_topic.in (),
                                                      DATAWRITER_QOS_DEFAULT,
                                                      DDS::DataWriterListener::_nil (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
       AckMessageDataWriter_var ackmessage_writer =
         AckMessageDataWriter::_narrow (dw.in());


       /* Create the subscriber */
       DDS::Subscriber_var s =
         dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                               DDS::SubscriberListener::_nil(),
                               ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);


       /* Initialize the transport for subscriber */
       OpenDDS::DCPS::TransportImpl_rch sub_tcp_impl;
       if (useTCP) {
         sub_tcp_impl
           = TheTransportFactory->create_transport_impl (TCP_IMPL_ID+1,
                                                         ACE_TEXT("tcp"),
                                                         ::OpenDDS::DCPS::AUTO_CONFIG);
       } else {
         sub_tcp_impl
           = TheTransportFactory->create_transport_impl(UDP_IMPL_ID+1,
                                                        ACE_TEXT("udp"),
                                                        OpenDDS::DCPS::DONT_AUTO_CONFIG);
         OpenDDS::DCPS::TransportInst_rch config
           = TheTransportFactory->create_configuration (UDP_IMPL_ID+1,
           ACE_TEXT("udp"));

         OpenDDS::DCPS::UdpInst* udp_config
           = static_cast <OpenDDS::DCPS::UdpInst*> (config.in ());

         ACE_TString addrStr(ACE_LOCALHOST);
         addrStr += ACE_TEXT(":1237");
         udp_config->local_address_.set(addrStr.c_str ());
         sub_tcp_impl->configure(config.in());
       }

       /* Attach the transport protocol with the subscribing entity */
       OpenDDS::DCPS::SubscriberImpl* sub_impl =
         dynamic_cast<OpenDDS::DCPS::SubscriberImpl*> (s.in ());
       sub_impl->attach_transport(sub_tcp_impl.in());


       /* Create topic for datareader */
       PubMessageTypeSupportImpl* pubmessage_dt = new PubMessageTypeSupportImpl;
       pubmessage_dt->register_type (dp.in (),
                                     "DDSPerfTest::PubMessage");
       DDS::Topic_var pubmessage_topic = dp->create_topic ("pubmessage_topic", // topic name
                                                           "DDSPerfTest::PubMessage", // topic type
                                                           TOPIC_QOS_DEFAULT,
                                                           DDS::TopicListener::_nil (),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       /* Create the listener for datareader */
       DDS::DataReaderListener_var listener (new PubDataReaderListenerImpl);
       PubDataReaderListenerImpl* listener_servant =
         dynamic_cast<PubDataReaderListenerImpl*>(listener.in());

       /* Create AckMessage datareader */
       DDS::DataReader_var dr = s->create_datareader (pubmessage_topic.in (),
                                                      DATAREADER_QOS_DEFAULT,
                                                      listener.in (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

       listener_servant->init(dr.in(), dw.in(), useZeroCopyRead);

       while (listener_servant->done () == 0)
       {
         ACE_OS::sleep (1);
       };


       std::cout << "Sub: shut down" << std::endl;
       /* Shut down domain entities */
       dp->delete_contained_entities ();
       dpf->delete_participant (dp.in ());
       TheServiceParticipant->shutdown ();

       return(0);

}

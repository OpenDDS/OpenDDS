// -*- C++ -*-
//
// $Id$

#include "common.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to  
// resolve the build problem that the class is not defined when 
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"


const char* MY_TOPIC    = "foo";
const char* MY_TOPIC_FOR_UDP = "fooudp";
const char* MY_TOPIC_FOR_MULTICAST = "foomulticast";
const char* MY_TYPE     = "Foo";
const char* MY_TYPE_FOR_UDP = "FooUdp";
const char* MY_TYPE_FOR_MULTICAST = "FooMulticast";
const ACE_TCHAR* reader_address_str = ACE_TEXT("localhost:0");
const ACE_TCHAR* writer_address_str = ACE_TEXT("localhost:0");
int reader_address_given = 0;
int writer_address_given = 0;

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int use_take = 0;
int num_samples_per_instance = 1;
int num_instances_per_writer = 1;
int num_datareaders = 1;
int num_datawriters = 1;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1;
// default to using TCP
int using_udp = 0;
int using_multicast = 0;
int sequence_length = 10;
int no_key = 0;
InstanceDataMap results;
ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> num_reads = 0;
long op_interval_ms = 0;
long blocking_ms = 0;
int mixed_trans = 0;
int test_bit = 0;

OpenDDS::DCPS::TransportImpl_rch reader_tcp_impl;
OpenDDS::DCPS::TransportImpl_rch reader_udp_impl;
OpenDDS::DCPS::TransportImpl_rch reader_multicast_impl;
OpenDDS::DCPS::TransportImpl_rch writer_tcp_impl;
OpenDDS::DCPS::TransportImpl_rch writer_udp_impl;
OpenDDS::DCPS::TransportImpl_rch writer_multicast_impl;

ACE_TString synch_file_dir;

// These files need to be unlinked in the run test script before and
// after running.
ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");


int init_reader_transport ()
{
  int status = 0;

  if (using_multicast)
    {
      reader_multicast_impl 
        = TheTransportFactory->create_transport_impl (SUB_TRAFFIC_MULTICAST, 
                                                      ACE_TEXT("multicast"), 
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);
      OpenDDS::DCPS::TransportConfiguration_rch reader_config 
        = TheTransportFactory->create_configuration (SUB_TRAFFIC_MULTICAST, ACE_TEXT("multicast"));

      if (reader_multicast_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_transport: sub MULTICAST")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  else
    {
      if (mixed_trans || using_udp)
        {
          reader_udp_impl 
            = TheTransportFactory->create_transport_impl (SUB_TRAFFIC_UDP, 
                                                          ACE_TEXT("SimpleUdp"), 
                                                          OpenDDS::DCPS::DONT_AUTO_CONFIG);
          OpenDDS::DCPS::TransportConfiguration_rch reader_config 
            = TheTransportFactory->create_configuration (SUB_TRAFFIC_UDP, ACE_TEXT("SimpleUdp"));

          if (reader_udp_impl->configure(reader_config.in()) != 0)
            {
              ACE_ERROR((LM_ERROR,
                        ACE_TEXT("(%P|%t) init_reader_transport: sub UDP")
                        ACE_TEXT(" Failed to configure the transport.\n")));
              status = 1;
            }
        }

      if (!using_udp)
        {
          reader_tcp_impl 
            = TheTransportFactory->create_transport_impl (SUB_TRAFFIC_TCP,
                                                          ACE_TEXT("SimpleTcp"), 
                                                          OpenDDS::DCPS::DONT_AUTO_CONFIG);

          OpenDDS::DCPS::TransportConfiguration_rch reader_config 
            = TheTransportFactory->create_configuration (SUB_TRAFFIC_TCP, ACE_TEXT("SimpleTcp"));

          OpenDDS::DCPS::SimpleTcpConfiguration* reader_tcp_config 
            = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (reader_config.in ());

          if (reader_address_given)
            {
              ACE_INET_Addr reader_address (reader_address_str);
              reader_tcp_config->local_address_ = reader_address;
              reader_tcp_config->local_address_str_ = reader_address_str;
            }
          // else use default address - OS assigned.

          if (reader_tcp_impl->configure(reader_config.in()) != 0)
            {
              ACE_ERROR((LM_ERROR,
                        ACE_TEXT("(%P|%t) init_reader_transport: sub TCP ")
                        ACE_TEXT(" Failed to configure the transport.\n")));
              status = 1;
            }
        }
    }

  return status;
}



int init_writer_transport ()
{
  int status = 0;

  if (using_multicast)
    {
      writer_multicast_impl 
        = TheTransportFactory->create_transport_impl (PUB_TRAFFIC_MULTICAST, 
                                                      ACE_TEXT("multicast"), 
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch writer_config 
        = TheTransportFactory->create_configuration (PUB_TRAFFIC_MULTICAST, ACE_TEXT("multicast"));

      if (writer_multicast_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_writer_transport: pub MULTICAST")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  else 
  {
    if (mixed_trans || using_udp)
    {
      writer_udp_impl 
        = TheTransportFactory->create_transport_impl (PUB_TRAFFIC_UDP, 
        ACE_TEXT("SimpleUdp"), 
        OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch writer_config 
        = TheTransportFactory->create_configuration (PUB_TRAFFIC_UDP, ACE_TEXT("SimpleUdp"));

      if (writer_udp_impl->configure(writer_config.in()) != 0)
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) init_writer_transport: pub UDP")
          ACE_TEXT(" Failed to configure the transport.\n")));
        status = 1;
      }

      if (using_udp)
        return status;
    }

    writer_tcp_impl 
      = TheTransportFactory->create_transport_impl (PUB_TRAFFIC_TCP,
                                                    ACE_TEXT("SimpleTcp"), 
                                                    OpenDDS::DCPS::DONT_AUTO_CONFIG);

    OpenDDS::DCPS::TransportConfiguration_rch writer_config 
      = TheTransportFactory->create_configuration (PUB_TRAFFIC_TCP, ACE_TEXT("SimpleTcp"));

    OpenDDS::DCPS::SimpleTcpConfiguration* writer_tcp_config 
      = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (writer_config.in ());

    if (writer_address_given)
      {
        ACE_INET_Addr writer_address (writer_address_str);
        writer_tcp_config->local_address_ = writer_address;
        writer_tcp_config->local_address_str_ = writer_address_str;
      }
      // else use default address - OS assigned.

    if (writer_tcp_impl->configure(writer_config.in()) != 0)
      {
        ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) init_writer_transport: pub TCP")
                  ACE_TEXT(" Failed to configure the transport.\n")));
        status = 1;
      }
   }

  return status;
}


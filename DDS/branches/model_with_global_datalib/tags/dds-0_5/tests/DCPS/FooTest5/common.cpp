// -*- C++ -*-
//
// $Id$

#include "common.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpTransport.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration_rch.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "dds/DCPS/transport/simpleUDP/SimpleUdpFactory.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpTransport.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration_rch.h"
#include "dds/DCPS/transport/simpleUDP/SimpleUdpConfiguration.h"


const char* MY_TOPIC    = "foo";
const char* MY_TOPIC_FOR_UDP = "fooudp";
const char* MY_TYPE     = "Foo";
const char* MY_TYPE_FOR_UDP = "FooUdp";
const char * reader_address_str = "";
const char * writer_address_str = "";
int reader_address_given = 0;
int writer_address_given = 0;

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int use_take = 0;
int num_samples_per_instance = 1;
int num_instances_per_writer = 1;
int num_datareaders = 1;
int num_datawriters = 1;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1;
// default to using TCP
int using_udp = 0;
int sequence_length = 10;
int no_key = 0;
InstanceDataMap results;
ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> num_reads = 0;
long op_interval_ms = 0;
long blocking_ms = 0;
int mixed_trans = 0;

TAO::DCPS::TransportImpl_rch reader_tcp_impl;
TAO::DCPS::TransportImpl_rch reader_udp_impl;
TAO::DCPS::TransportImpl_rch writer_tcp_impl;
TAO::DCPS::TransportImpl_rch writer_udp_impl;

ACE_TString synch_file_dir;

// These files need to be unlinked in the run test script before and
// after running.
ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");


int init_reader_tranport ()
{
  int status = 0;

  if (mixed_trans || using_udp)
    {
      TheTransportFactory->register_type(SIMPLE_UDP,
                                     new TAO::DCPS::SimpleUdpFactory());

      TAO::DCPS::SimpleUdpConfiguration_rch reader_config =
          new TAO::DCPS::SimpleUdpConfiguration();

      if (!reader_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 11;
        }


      ACE_INET_Addr reader_address (reader_address_str);
      reader_config->local_address_ = reader_address;

      reader_udp_impl =
          TheTransportFactory->create(SUB_TRAFFIC_UDP,
                                      SIMPLE_UDP);

      if (reader_udp_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub UDP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  if (mixed_trans || ! using_udp)
    {
      TheTransportFactory->register_type(SIMPLE_TCP,
                                         new TAO::DCPS::SimpleTcpFactory());

      TAO::DCPS::SimpleTcpConfiguration_rch reader_config =
          new TAO::DCPS::SimpleTcpConfiguration();

      if (reader_address_given)
        {
          ACE_INET_Addr reader_address (reader_address_str);
          reader_config->local_address_ = reader_address;
        }
        // else use default address - OS assigned.

      reader_tcp_impl =
          TheTransportFactory->create(SUB_TRAFFIC_TCP,
                                      SIMPLE_TCP);

      if (reader_tcp_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_reader_tranport: sub TCP ")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  return status;
}



int init_writer_tranport ()
{
  int status = 0;

  if (mixed_trans || using_udp)
    {
      TheTransportFactory->register_type(SIMPLE_UDP,
                                         new TAO::DCPS::SimpleUdpFactory());

      TAO::DCPS::SimpleUdpConfiguration_rch writer_config =
          new TAO::DCPS::SimpleUdpConfiguration();

      writer_udp_impl =
          TheTransportFactory->create(PUB_TRAFFIC_UDP,
                                      SIMPLE_UDP);

      if (!writer_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_writer_tranport: pub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 12;
        }

      ACE_INET_Addr writer_address (writer_address_str);
      writer_config->local_address_ = writer_address;

      if (writer_udp_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_writer_tranport: pub UDP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  if (mixed_trans || ! using_udp)
    {
      TheTransportFactory->register_type(SIMPLE_TCP,
                                         new TAO::DCPS::SimpleTcpFactory());

      TAO::DCPS::SimpleTcpConfiguration_rch writer_config =
          new TAO::DCPS::SimpleTcpConfiguration();

      writer_tcp_impl =
          TheTransportFactory->create(PUB_TRAFFIC_TCP,
                                      SIMPLE_TCP);

      if (writer_address_given)
        {
          ACE_INET_Addr writer_address (writer_address_str);
          writer_config->local_address_ = writer_address;
        }
        // else use default address - OS assigned.

      if (writer_tcp_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_writer_tranport: pub TCP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  return status;
}


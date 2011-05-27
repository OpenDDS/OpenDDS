// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleMcastConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"





const long  TEST_DOMAIN   = 411;
const char* TEST_TOPIC    = "McastListenerPerfTest";
const char* TEST_TYPE     = "McastListenerPerfTestType";
const char * reader_address_str = "default";
const char * writer_address_str = "default";
const char * multicast_group_address_str = "default";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int NUM_SAMPLES = 128;
int DATA_SIZE = 128;
int RECVS_BTWN_READS = 10;
CORBA::ULong max_mili_sec_blocking = 5000;

int num_datawriters = 1;
int num_datareaders = 1;
unsigned throttle_factor = 0;
unsigned id = 0;
// largest positive value of a long is 2147483647
CORBA::Long MAX_SAMPLES_PER_INSTANCE = ::DDS::LENGTH_UNLIMITED; 
CORBA::Long MAX_SAMPLES = ::DDS::LENGTH_UNLIMITED;
CORBA::Long MAX_INSTANCES = ::DDS::LENGTH_UNLIMITED;

TAO::DCPS::TransportImpl_rch reader_transport_impl;
TAO::DCPS::TransportImpl_rch writer_transport_impl;

enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};



int init_reader_tranport ()
{
  int status = 0;

  reader_transport_impl =
      TheTransportFactory->create_transport_impl (SUB_TRAFFIC, "SimpleMcast", TAO::DCPS::DONT_AUTO_CONFIG);
  TAO::DCPS::TransportConfiguration_rch reader_config 
    = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleMcast");

  TAO::DCPS::SimpleMcastConfiguration* reader_mcast_config 
    = static_cast <TAO::DCPS::SimpleMcastConfiguration*> (reader_config.in ());

  if (0 != ACE_OS::strcmp("default", reader_address_str) )
    {
      ACE_INET_Addr reader_address (reader_address_str);
      //reader_mcast_config->local_address_ = reader_address;
    }

  if (0 != ACE_OS::strcmp("default", multicast_group_address_str) )
    {
      ACE_INET_Addr multicast_group_address (multicast_group_address_str);
      reader_mcast_config->multicast_group_address_ = multicast_group_address;
    }

  reader_mcast_config->receiver_ = true;

  if (reader_transport_impl->configure(reader_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ::init_reader_tranport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }

  return status;
}



int init_writer_tranport ()
{
  int status = 0;

  writer_transport_impl =
      TheTransportFactory->create_transport_impl(PUB_TRAFFIC, 
                                                 "SimpleMcast",
                                                 TAO::DCPS::DONT_AUTO_CONFIG);

  TAO::DCPS::TransportConfiguration_rch writer_config 
    = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleMcast");

  TAO::DCPS::SimpleMcastConfiguration* writer_mcast_config 
    = static_cast <TAO::DCPS::SimpleMcastConfiguration*> (writer_config.in ());

  if (0 != ACE_OS::strcmp("default", writer_address_str) )
    {
      ACE_INET_Addr writer_address (writer_address_str);
      //writer_mcast_config->local_address_ = writer_address;
    }

  if (0 != ACE_OS::strcmp("default", multicast_group_address_str) )
    {
      ACE_INET_Addr multicast_group_address (multicast_group_address_str);
      writer_mcast_config->multicast_group_address_ = multicast_group_address;
    }

  if (writer_transport_impl->configure(writer_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::init_writer_tranport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }

  return status;
}


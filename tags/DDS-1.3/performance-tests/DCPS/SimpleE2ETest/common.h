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


#include "dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpTransport.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration_rch.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"





const long  TEST_DOMAIN   = 411;
const char* TEST_TOPIC    = (const char* ) "PerfTest";
const char* TEST_TYPE     = (const char* ) "PerfTestType";
const ACE_TCHAR* reader_address_str = ACE_TEXT("default");
const ACE_TCHAR* writer_address_str = ACE_TEXT("default");

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int NUM_SAMPLES = 128;
int DATA_SIZE = 128;
CORBA::ULong max_mili_sec_blocking = 5000;

int num_datawriters = 1;
// largest positive value of a long is 2147483647
CORBA::Long MAX_SAMPLES_PER_INSTANCE = ::DDS::LENGTH_UNLIMITED; 
CORBA::Long MAX_SAMPLES = ::DDS::LENGTH_UNLIMITED;
CORBA::Long MAX_INSTANCES = ::DDS::LENGTH_UNLIMITED;

OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

enum TransportTypeId
{
  SIMPLE_TCP
};

enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};



int init_reader_tranport ()
{
  int status = 0;

  reader_transport_impl =
      TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                  ACE_TEXT("SimpleTcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);
  
  OpenDDS::DCPS::TransportConfiguration_rch reader_config 
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, ACE_TEXT("SimpleTcp"));
      
  OpenDDS::DCPS::SimpleTcpConfiguration* reader_tcp_config
        = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (reader_config.in ());
      
  if (0 != ACE_OS::strcmp(ACE_TEXT("default"), reader_address_str) )
    {
      ACE_INET_Addr reader_address (reader_address_str);
      reader_tcp_config->local_address_ = reader_address;
    }

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
      TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                  ACE_TEXT("SimpleTcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);
  
  OpenDDS::DCPS::TransportConfiguration_rch writer_config 
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, ACE_TEXT("SimpleTcp"));
      
  OpenDDS::DCPS::SimpleTcpConfiguration* writer_tcp_config
        = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (writer_config.in ());
      
  if (0 != ACE_OS::strcmp(ACE_TEXT("default"), writer_address_str) )
    {
      ACE_INET_Addr writer_address (writer_address_str);
      writer_tcp_config->local_address_ = writer_address;
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



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


#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUdpConfiguration.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include "ace/SString.h"

#define MY_DOMAIN 411

#define MY_TOPIC1  "T1"
#define MY_TOPIC2  "T2"
#define MY_TOPIC3  "T3"

#define MY_TYPE1 "foo1"
#define MY_TYPE2 "foo2"
#define MY_TYPE3 "foo3"

#define  TOPIC_T1  1
#define  TOPIC_T2  2
#define  TOPIC_T3  4

static const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

static const int LEASE_DURATION_SEC = 5 ; // seconds

static int num_ops_per_thread = 10;
static int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
static int history_depth = 100 ;
// default to using TCP
static int using_udp = 0;

// These files need to be unlinked in the run test script before and
// after running.
//static ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
static ACE_TString pub_finished_filename = ACE_LIB_TEXT("_publisher_finished.txt");
//static ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
static ACE_TString sub_finished_filename = ACE_LIB_TEXT("_subscriber_finished.txt");

enum TransportTypeId
{
  SIMPLE_TCP,
  SIMPLE_UDP
};


enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};

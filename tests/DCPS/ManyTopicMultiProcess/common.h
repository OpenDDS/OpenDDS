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


#include "dds/DCPS/transport/tcp/TcpInst.h"

#include "dds/DCPS/transport/udp/UdpInst.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include "ace/SString.h"

#define MY_DOMAIN 4

#define MY_TOPIC1 "T1"
#define MY_TOPIC2 "T2"
#define MY_TOPIC3 "T3"
#define MY_TOPIC4 "T4"
#define MY_TOPIC5 "T5"
#define MY_TOPIC6 "T6"
#define MY_TOPIC7 "T7"

#define MY_TYPE1 "foo1"
#define MY_TYPE4 "foo4"

#define TOPIC_T1 1
#define TOPIC_T2 2
#define TOPIC_T3 4
#define TOPIC_T4 8
#define TOPIC_T5 16
#define TOPIC_T6 32
#define TOPIC_T7 64

static const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

static const int LEASE_DURATION_SEC = 5; // seconds

// These files need to be unlinked in the run test script before and
// after running.
//static ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
static ACE_TString pub_finished_filename = ACE_TEXT("_publisher_finished.txt");
//static ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
static ACE_TString sub_finished_filename = ACE_TEXT("_subscriber_finished.txt");


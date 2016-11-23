#ifndef _COMMON_H_
#define _COMMON_H_
// -*- C++ -*-
// ============================================================================
/**
 *  @file  common.h
 *
 *
 *
 */
// ============================================================================


#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"

#include <map>


static const long  MY_DOMAIN   = 411;

#define MY_TOPIC "foo"
#define MY_TYPE  "foo"

extern ACE_Time_Value max_blocking_time ;

static const int I1 = 1;
static const int I2 = 2;
static const int I3 = 3;

extern OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
extern OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

typedef std::map < char, ::DDS::SampleInfo > SampleInfoMap ;

enum TransportTypeId
{
  SIMPLE_TCP
};

enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};

#endif


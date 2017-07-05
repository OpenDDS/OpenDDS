// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
 *
 *
 */
// ============================================================================

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#ifndef COMMON_H_BB55E14B
#define COMMON_H_BB55E14B

#include "ace/SString.h"
#include "dds/DCPS/PoolAllocator.h"

#define MY_DOMAIN 211
#define MY_TOPIC  "foo"
#define MY_TYPE "foo"


// These files need to be unlinked in the run test script before and
// after running.
static ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
static ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
static ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
static ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");

enum TransportTypeId
{
  SIMPLE_TCP
};


enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};


//=== these may be changed by options so they are in common.cpp
//=== so changes will not be local to the file that included them.

extern DDS::Duration_t LEASE_DURATION;

extern int test_duration;
extern ::DDS::ReliabilityQosPolicyKind reliability_kind;
extern ::DDS::DurabilityQosPolicyKind durability_kind;
extern ::DDS::LivelinessQosPolicyKind liveliness_kind;
extern bool compatible;

extern ACE_TString LEASE_DURATION_STR;
extern std::string test_duration_str;
extern ACE_TString reliability_kind_str;
extern ACE_TString durability_kind_str;
extern ACE_TString liveliness_kind_str;

::DDS::DurabilityQosPolicyKind get_durability_kind(const ACE_TString& argument);
::DDS::LivelinessQosPolicyKind get_liveliness_kind(const ACE_TString& argument);
::DDS::ReliabilityQosPolicyKind get_reliability_kind(const ACE_TString& argument);
DDS::Duration_t get_lease_duration(const ACE_TString& argument);

#endif /* end of include guard: COMMON_H_BB55E14B */


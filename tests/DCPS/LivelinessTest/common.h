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

#include "ace/SString.h"

#define MY_DOMAIN 411
#define MY_TOPIC  "foo"
#define MY_TYPE "foo"


// These files need to be unlinked in the run test script before and
// after running.
static ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
static ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
static ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
static ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");


//=== these may be changed by options so they are in common.cpp
//=== so changes will not be local to the file that included them.

extern int LEASE_DURATION_SEC; // seconds

extern int num_ops_per_thread;
extern int num_unlively_periods;
extern int max_samples_per_instance;
extern int history_depth;
extern int use_take;

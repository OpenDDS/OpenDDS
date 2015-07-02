// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
 *
 *
 */
// ============================================================================


#include "dds/DCPS/transport/tcp/TcpInst.h"


const long  TEST_DOMAIN   = 411;
const char* TEST_TOPIC    = "PerfTest";
const char* TEST_TYPE     = "PerfTestType";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int NUM_SAMPLES = 128;
int DATA_SIZE = 128;
int RECVS_BTWN_READS = 10;
CORBA::ULong max_mili_sec_blocking = 5000;
long subscriber_delay_msec = 0;

int num_datawriters = 1;
int num_datareaders = 1;
bool use_zero_copy_reads = true;
unsigned id = 0;
// largest positive value of a long is 2147483647
CORBA::Long MAX_SAMPLES_PER_INSTANCE = ::DDS::LENGTH_UNLIMITED;
CORBA::Long MAX_SAMPLES = ::DDS::LENGTH_UNLIMITED;
CORBA::Long MAX_INSTANCES = ::DDS::LENGTH_UNLIMITED;


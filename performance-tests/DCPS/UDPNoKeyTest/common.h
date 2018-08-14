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

const long  TEST_DOMAIN   = 111;
const char* TEST_TOPIC    = "PerfTest";
const char* TEST_TYPE     = "PerfTestType";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int NUM_SAMPLES = 128;
int DATA_SIZE = 128;
CORBA::ULong max_mili_sec_blocking = 5000;

int num_datawriters = 1;
int num_datareaders = 1;
unsigned throttle_factor = 0;
unsigned id = 0;
// largest positive value of a long is 2147483647
CORBA::Long MAX_SAMPLES_PER_INSTANCE = ::DDS::LENGTH_UNLIMITED;
CORBA::Long MAX_SAMPLES = ::DDS::LENGTH_UNLIMITED;
CORBA::Long MAX_INSTANCES = ::DDS::LENGTH_UNLIMITED;


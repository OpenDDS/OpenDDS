// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "dds/DdsDcpsInfrastructureC.h"

int LEASE_DURATION_SEC = 2 ; // seconds

int num_ops_per_thread = 100;
int num_unlively_periods = 10;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1000 ;
// default to using TCP
int using_udp = 0;
int use_take = 0;


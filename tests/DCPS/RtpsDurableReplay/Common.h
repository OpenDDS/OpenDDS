// -*- C++ -*-
//
#ifndef COMMON_H
#define COMMON_H

#include <dds/DdsDcpsInfrastructureC.h>

const DDS::DomainId_t TEST_DOMAIN = 111;
const size_t INSTANCE_COUNT = 10;

const char PUBLISHER_ACTOR[] = "publisher";
const char SUBSCRIBER_ACTOR[] = "subscriber";
const char ALL_INSTANCES_RECEIVED_BEFORE[] = "all_instances_received_before";
const char UNMATCHED[] = "unmatched";
const char RESUME[] = "resume";
const char ALL_INSTANCES_RECEIVED_AFTER[] = "all_instances_received_after";

#endif /* COMMON_H */

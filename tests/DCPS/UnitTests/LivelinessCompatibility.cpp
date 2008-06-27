// -*- C++ -*-
// ============================================================================
/**
 *  @file   LivelinessCompatibility.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "../common/TestSupport.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Qos_Helper.h"

bool
lease_greater_than (::DDS::LivelinessQosPolicy const & qos1,
                    ::DDS::LivelinessQosPolicy const & qos2)
{
  return qos1.lease_duration > qos2.lease_duration;
}


int main (int , char *[])
{
  int status = 0;

  // verify that if dw is infinite, that it is always greater
  // (except when dr is also infinite, covered next)
  {
    ::DDS::LivelinessQosPolicy dw_liveliness;
    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC;

    ::DDS::LivelinessQosPolicy dr_liveliness;
    dr_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
    dr_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC + 1;

    // verify that the datawriter liveliness is greater, since it is infinite
    TEST_CHECK(lease_greater_than(dw_liveliness, dr_liveliness))
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness))

    dr_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_NSEC;
    dr_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC - 1;

    // verify that the datawriter liveliness is greater, since it is infinite
    TEST_CHECK(lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness));

    dr_liveliness.lease_duration.sec = 0;
    dr_liveliness.lease_duration.nanosec = 0;

    // verify that the datawriter liveliness is greater, since it is infinite
    TEST_CHECK(lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness));
  }

  // verify that if dr is infinite, that dw is never greater
  {
    ::DDS::LivelinessQosPolicy dw_liveliness;
    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC;

    ::DDS::LivelinessQosPolicy dr_liveliness;
    dr_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
    dr_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC;

    // verify that the datawriter liveliness isn't greater, since they are equal
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC + 1;

    // verify that the datawriter liveliness isn't greater, since dr is infinite
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC - 1;

    // verify that the datawriter liveliness isn't greater, since dr is infinite
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = 0;
    dw_liveliness.lease_duration.nanosec = 0;

    // verify that the datawriter liveliness isn't greater, since dr is infinite
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(lease_greater_than(dr_liveliness, dw_liveliness));
  }

  // neither is infinite
  {
    ::DDS::LivelinessQosPolicy dw_liveliness;
    dw_liveliness.lease_duration.sec = 5;
    dw_liveliness.lease_duration.nanosec = 0;

    ::DDS::LivelinessQosPolicy dr_liveliness;
    dr_liveliness.lease_duration.sec = 5;
    dr_liveliness.lease_duration.nanosec = 0;

    // verify that the datawriter liveliness isn't greater
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec += 1;

    // verify that the datawriter liveliness is greater
    TEST_CHECK(lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec -= 2;

    // verify that the datawriter liveliness isn't greater
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = 15;
    dw_liveliness.lease_duration.nanosec = 16;

    dr_liveliness.lease_duration.sec = 15;
    dr_liveliness.lease_duration.nanosec = 15;

    // verify that the datawriter liveliness is greater
    TEST_CHECK(lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.nanosec -= 2;

    // verify that the datawriter liveliness isn't greater
    TEST_CHECK(!lease_greater_than(dw_liveliness, dr_liveliness));
    TEST_CHECK(lease_greater_than(dr_liveliness, dw_liveliness));
  }

  return status;
}

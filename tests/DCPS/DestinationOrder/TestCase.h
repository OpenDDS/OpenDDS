/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TEST_DESTINATIONORDER_H
#define DCPS_TEST_DESTINATIONORDER_H

#include "TestFramework_T.h"

class TestCase : public virtual TestPair<TestMessageDataReader,
                                         TestMessageDataWriter> {
public:
  virtual DDS::ReturnCode_t init_datareader(
    DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr& listener,
    DDS::StatusMask& status);

  virtual int test();
};

#endif  /* DCPS_TEST_DESTINATIONORDER_H */

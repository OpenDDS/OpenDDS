/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TEST_DESTINATIONORDER_H
#define DCPS_TEST_DESTINATIONORDER_H

#include "TestFramework_T.h"

class TestCase : public TestBase {
public:
  virtual DDS::ReturnCode_t init_datareader(
    DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr& listener,
    DDS::StatusMask& status);

  virtual void init_i(int, ACE_TCHAR*[]);
  virtual void fini_i();
  virtual int test();

  void wait_for_subscribers();

private:
  typedef TestPublisher<TestMessageDataWriter>  TestPublisherType;
  typedef TestSubscriber<TestMessageDataReader> TestSubscriberType;
  TestPublisherType*  publisher_;
  TestSubscriberType* subscriber_;
};

#endif  /* DCPS_TEST_DESTINATIONORDER_H */

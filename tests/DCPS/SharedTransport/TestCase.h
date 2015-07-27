/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TEST_SHAREDTRANSPORT_H
#define DCPS_TEST_SHAREDTRANSPORT_H

#include "TestFramework_T.h"

class TestCase : public TestBase {
public:
  TestCase();

  virtual DDS::ReturnCode_t init_datawriter(
    DDS::DataWriterQos& qos,
    DDS::DataWriterListener_ptr& listener,
    DDS::StatusMask& status);

  virtual DDS::ReturnCode_t init_datareader(
    DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr& listener,
    DDS::StatusMask& status);

  DDS::ReturnCode_t init_transport(const std::string& transport_type);

  typedef TestPublisher<TestMessageDataWriter>  TestPublisherType;
  typedef TestSubscriber<TestMessageDataReader> TestSubscriberType;
  typedef std::vector<TestPublisherType*>  TestPublisherVector;
  typedef std::vector<TestSubscriberType*> TestSubscriberVector;

  TestPublisherVector  publishers_;
  TestSubscriberVector subscribers_;

  void wait_for_subscribers();

  virtual void init_i(int argc, ACE_TCHAR* argv[]);
  virtual void fini_i();

  int test();

  void set_writers(int i);

private:
  int num_writers_;
  int num_readers_;
};

#endif  /* DCPS_TEST_SHAREDTRANSPORTORDER_H */

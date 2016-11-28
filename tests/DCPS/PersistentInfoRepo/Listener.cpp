/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Listener.h"
#include "ace/OS_NS_unistd.h"

Listener::Listener() :
  sample_count_(-1), expected_count_(0), expected_seq_(0)
{
}

void Listener::on_sample(const ::Xyz::Foo& msg)
{
  if (sample_count_ == -1) {
    expected_count_ = msg.key;
  } else if (msg.key != expected_count_) {
    std::cout << "Error: expected_count changed to " << msg.key
              << std::endl;
  }

  if (expected_seq_ != msg.c) {
    std::cout << "Expected: " << expected_seq_
              << " Received: " << msg.c << std::endl;
  }
  ++sample_count_;
  if (((sample_count_ + 1) % 1000) == 0) {
    std::cout << "Got sample " << sample_count_ + 1 << " sleeping" << std::endl;
    ACE_OS::sleep(2);
  }

  // Next message
  expected_seq_ = msg.c + 1;
}

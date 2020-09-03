/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TestObserver.h"
#include <dds/DCPS/DataWriterImpl.h>
#include <dds/DCPS/DataReaderImpl.h>

#include <ace/OS_NS_time.h>

#include <ctime>
#include <iostream>
#include <sstream>

std::string TestObserver::to_str(DDS::DataWriter_ptr w)
{
  std::ostringstream o;
  o << " writer ";
  auto p = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(w);
  if (p) {
    o << p->get_publication_id();
  }
  return o.str();
}

std::string TestObserver::to_str(DDS::DataReader_ptr r)
{
  std::ostringstream o;
  o << " reader ";
  auto p = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(r);
  if (p) {
    o << p->get_subscription_id();
  }
  return o.str();
}

std::string TestObserver::to_str(const Sample& s)
{
  const std::string t("    ");
  std::time_t seconds = OpenDDS::DCPS::time_to_time_value(s.timestamp_).sec();
  ACE_TCHAR buffer[32];
  std::ostringstream o; o << '\n'
    << t << "Timestamp: " << ACE_OS::ctime_r(&seconds, buffer, 32)
    << t << "Instance : " << s.instance_ << '\n'
    << t << "State    : " << s.instance_state_ << '\n'
    << t << "SequenceN: " << s.seq_n_.getValue() << '\n'
    << t << "Data     : " << s.data_ << '\n';
  return o.str();
}

// ========== ========== ========== ========== ========== ========== ==========
// 1. Writer and Reader: enabled, deleted, qos_changed

void TestObserver::on_enabled(DDS::DataWriter_ptr w)
{
  std::cout << "on_enabled" << to_str(w) << std::endl;
}

void TestObserver::on_enabled(DDS::DataReader_ptr r)
{
  std::cout << "on_enabled" << to_str(r) << std::endl;
}

void TestObserver::on_deleted(DDS::DataWriter_ptr w)
{
  std::cout << "on_deleted" << to_str(w) << std::endl;
}

void TestObserver::on_deleted(DDS::DataReader_ptr r)
{
  std::cout << "on_deleted" << to_str(r) << std::endl;
}

void TestObserver::on_qos_changed(DDS::DataWriter_ptr w)
{
  std::cout << "on_qos_changed" << to_str(w) << std::endl;
}

void TestObserver::on_qos_changed(DDS::DataReader_ptr r)
{
  std::cout << "on_qos_changed" << to_str(r) << std::endl;
}

// ========== ========== ========== ========== ========== ========== ==========
// 2. Association: associated, disassociated, associated_qos_changed

void TestObserver::on_associated(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId)
{
  std::cout << "on_associated" << to_str(w) << " with reader " << readerId << std::endl;
}

void TestObserver::on_associated(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId)
{
  std::cout << "on_associated" << to_str(r) << " with writer " << writerId << std::endl;
}

void TestObserver::on_disassociated(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId)
{
  std::cout << "on_disassociated" << to_str(w) << " from reader " << readerId << std::endl;
}

void TestObserver::on_disassociated(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId)
{
  std::cout << "on_disassociated" << to_str(r) << " from writer " << writerId << std::endl;
}

void TestObserver::on_associated_qos_changed(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId)
{
  std::cout << "on_associated_qos_changed" << to_str(w) << " with reader " << readerId << std::endl;
}

void TestObserver::on_associated_qos_changed(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId)
{
  std::cout << "on_associated_qos_changed" << to_str(r) << " with writer " << writerId << std::endl;
}

// ========== ========== ========== ========== ========== ========== ==========
// 3. Sample: sent, received, read, taken

void TestObserver::on_sample_sent(const DDS::DataWriter_ptr w, const Sample& s)
{
  std::cout << "on_sample_sent" << to_str(w) << to_str(s);
}

void TestObserver::on_sample_received(const DDS::DataReader_ptr r, const Sample& s)
{
  std::cout << "on_sample_received" << to_str(r) << to_str(s);
}

void TestObserver::on_sample_read(const DDS::DataReader_ptr r, const Sample& s)
{
  std::cout << "on_sample_read" << to_str(r) << to_str(s);
}

void TestObserver::on_sample_taken(const DDS::DataReader_ptr r, const Sample& s)
{
  std::cout << "on_sample_taken" << to_str(r) << to_str(s);
}

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TESTOBSERVER_H
#define OPENDDS_DCPS_TESTOBSERVER_H

#include <dds/DCPS/Observer.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class TestObserver : public virtual OpenDDS::DCPS::Observer
{
public:
  TestObserver() : Observer() {}
  virtual ~TestObserver() {}

  virtual void on_enabled(DDS::DataWriter_ptr w);
  virtual void on_enabled(DDS::DataReader_ptr r);
  virtual void on_deleted(DDS::DataWriter_ptr w);
  virtual void on_deleted(DDS::DataReader_ptr r);
  virtual void on_qos_changed(DDS::DataWriter_ptr w);
  virtual void on_qos_changed(DDS::DataReader_ptr r);

  virtual void on_associated(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId);
  virtual void on_associated(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId);
  virtual void on_disassociated(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId);
  virtual void on_disassociated(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId);
  virtual void on_associated_qos_changed(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId);
  virtual void on_associated_qos_changed(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId);

  virtual void on_sample_sent(const DDS::DataWriter_ptr w, const Sample& s);
  virtual void on_sample_received(const DDS::DataReader_ptr r, const Sample& s);
  virtual void on_sample_read(const DDS::DataReader_ptr r, const Sample& s);
  virtual void on_sample_taken(const DDS::DataReader_ptr r, const Sample& s);

private:
  static std::string to_str(DDS::DataWriter_ptr w);
  static std::string to_str(DDS::DataReader_ptr w);
  static std::string to_str(const Sample& s);
};

#endif // OPENDDS_DCPS_TESTOBSERVER_H

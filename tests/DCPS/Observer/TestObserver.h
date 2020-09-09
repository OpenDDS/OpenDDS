/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TESTOBSERVER_H
#define OPENDDS_DCPS_TESTOBSERVER_H

#ifndef ACE_LACKS_PRAGMA_ONCE
#pragma once
#endif

#include <dds/DCPS/Observer.h>

#include <sstream>

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

  virtual void on_sample_sent(const DDS::DataWriter_ptr w, const Sample& s);
  virtual void on_sample_received(const DDS::DataReader_ptr r, const Sample& s);
  virtual void on_sample_read(const DDS::DataReader_ptr r, const Sample& s);
  virtual void on_sample_taken(const DDS::DataReader_ptr r, const Sample& s);

private:
  static std::string to_str(DDS::DataWriter_ptr w);
  static std::string to_str(DDS::DataReader_ptr w);
  static std::string to_str(const Sample& s);
  static std::string to_str(const OpenDDS::DCPS::GUID_t& guid);

  template<typename Qos>
  static std::string qos_str(const Qos& qos) {
    std::ostringstream o; o << " qos.user_data: ";
    for (CORBA::ULong i = 0; i < qos.user_data.value.length(); ++i) {
      o << qos.user_data.value[i];
    }
    o << '\n';
    return o.str();
  }

  struct Stats
  {
    unsigned int w_enabled_, w_deleted_, w_qos_changed_; // group 1 writer
    unsigned int r_enabled_, r_deleted_, r_qos_changed_; // group 1 reader
    unsigned int w_associated_, w_disassociated_; // group 2 writer
    unsigned int r_associated_, r_disassociated_; // group 2 reader
    unsigned int sent_, received_, read_, taken_; // group 3 writer/reader

    Stats() :
      w_enabled_(0), w_deleted_(0), w_qos_changed_(0),
      r_enabled_(0), r_deleted_(0), r_qos_changed_(0),
      w_associated_(0) , w_disassociated_(0),
      r_associated_(0) , r_disassociated_(0),
      sent_(0), received_(0), read_(0), taken_(0) {}

    bool w_g1_g2() const;
    bool r_g1_g2() const;
  };
  Stats stats_;
  //lock
};

#endif // OPENDDS_DCPS_TESTOBSERVER_H

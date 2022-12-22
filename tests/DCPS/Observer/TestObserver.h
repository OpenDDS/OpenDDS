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
#include <dds/DCPS/EntityImpl.h>

#include <sstream>

class TestObserver : public virtual OpenDDS::DCPS::Observer
{
public:
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

  virtual void on_sample_sent(DDS::DataWriter_ptr w,
                              const Sample& s);
  virtual void on_sample_received(DDS::DataReader_ptr r,
                                  const Sample& s);
  virtual void on_sample_read(DDS::DataReader_ptr r,
                              const Sample& s);
  virtual void on_sample_taken(DDS::DataReader_ptr r,
                               const Sample& s);

  enum Check { c_SENT = 1, c_W_G1_G2 = 2, c_R_ALL = 3 };

  explicit TestObserver(Check check = c_SENT);
  virtual ~TestObserver();

  enum {
    n_WRITER = 1, n_READER = 2, n_MSG = 6,
    n_ASSOCIATION = n_WRITER * n_READER,
    n_RECEIVED = n_MSG * n_READER,
    n_READ = 3, n_TAKEN = n_MSG - n_READ
  };

private:
  static std::string to_str(DDS::DataWriter_ptr w);
  static std::string to_str(DDS::DataReader_ptr w);
  static std::string to_str(const OpenDDS::DCPS::GUID_t& guid);

  template<typename Qos>
  static std::string qos_str(const Qos& qos) {
    std::ostringstream o;
    o << " qos.user_data: ";
    for (CORBA::ULong i = 0; i < qos.user_data.value.length(); ++i) {
      o << qos.user_data.value[i];
    }
    o << '\n';
    return o.str();
  }

  bool check_w_g1_g2() const;
  bool check_r_all() const;
  void show_observed(const std::string& txt) const;

  const Check check_;

  int w_enabled_, w_deleted_, w_qos_changed_; // group 1 writer
  int r_enabled_, r_deleted_, r_qos_changed_; // group 1 reader
  int w_associated_, w_disassociated_; // group 2 writer
  int r_associated_, r_disassociated_; // group 2 reader
  int sent_, received_, read_, taken_; // group 3 writer/reader
};

#endif // OPENDDS_DCPS_TESTOBSERVER_H

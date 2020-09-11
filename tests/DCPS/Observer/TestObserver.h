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
  TestObserver();
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

  enum {
    w_ENABLED = 1, w_DELETED = 0, w_QOS_CHANGED = 1, w_ASSOCIATED = 2, w_DISASSOCIATED = 0,
    r_ENABLED = 2, r_DELETED = 0, r_QOS_CHANGED = 2, r_ASSOCIATED = 2, r_DISASSOCIATED = 2,
    n_SENT = 6, n_RECEIVED = 16, n_READ = 3, n_TAKEN = 4
  };

  bool w_g1_g2() const;
  bool r_g1_g2() const;
  bool sent() const { return n_SENT == sent_; }
  bool received() const { return n_RECEIVED == received_; }
  bool read_taken() const { return n_READ == read_ && n_TAKEN == taken_; }

  template<typename E>
  static TestObserver* get(E* entity, const Observer::Event e) {
    OpenDDS::DCPS::EntityImpl* i = dynamic_cast<OpenDDS::DCPS::EntityImpl*>(entity);
    Observer* o = i ? i->get_observer(e).get() : 0;
    return o ? dynamic_cast<TestObserver*>(o) : 0;
  }

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

  int w_enabled_, w_deleted_, w_qos_changed_; // group 1 writer
  int r_enabled_, r_deleted_, r_qos_changed_; // group 1 reader
  int w_associated_, w_disassociated_; // group 2 writer
  int r_associated_, r_disassociated_; // group 2 reader
  int sent_, received_, read_, taken_; // group 3 writer/reader
};

#endif // OPENDDS_DCPS_TESTOBSERVER_H

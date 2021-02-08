/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TestObserver.h"

#include <dds/DCPS/DataReaderImpl.h>
#include <dds/DCPS/DataWriterImpl.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/JsonValueWriter.h>

#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <ace/OS_NS_time.h>

#include <ctime>
#include <iostream>
#include <sstream>

TestObserver::TestObserver(Check check) : Observer()
  , check_(check)
  , w_enabled_(0), w_deleted_(0), w_qos_changed_(0)
  , r_enabled_(0), r_deleted_(0), r_qos_changed_(0)
  , w_associated_(0), w_disassociated_(0)
  , r_associated_(0), r_disassociated_(0)
  , sent_(0), received_(0), read_(0), taken_(0)
{}

TestObserver::~TestObserver()
{
  std::cout << "check_ = " << check_ << '\n';
  if (check_ == c_SENT) {
    if (sent_ != n_MSG) {
      show_observed("sent incorrect");
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: sent = %d.\n"), sent_));
    }
  } else if (check_ == c_W_G1_G2) {
    if (!check_w_g1_g2()) {
      show_observed("check_w_g1_g2() failed");
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: check_w_g1_g2() failed!\n")));
    }
  } else if (!check_r_all()) {
    show_observed("check_r_all() failed");
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: check_r_all() failed!\n")));
  }
}

std::string TestObserver::to_str(DDS::DataWriter_ptr w)
{
  std::ostringstream o;
  o << " writer ";
  OpenDDS::DCPS::DataWriterImpl* p = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(w);
  if (p) {
    o << to_str(p->get_repo_id());
  }
  return o.str();
}

std::string TestObserver::to_str(DDS::DataReader_ptr r)
{
  std::ostringstream o;
  o << " reader ";
  OpenDDS::DCPS::DataReaderImpl* p = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(r);
  if (p) {
    o << to_str(p->get_repo_id());
  }
  return o.str();
}

std::string TestObserver::to_str(const OpenDDS::DCPS::GUID_t& guid)
{
  return OpenDDS::DCPS::to_string(guid).c_str();
}

// ========== ========== ========== ========== ========== ========== ==========
// 1. Writer and Reader: enabled, deleted, qos_changed

void TestObserver::on_enabled(DDS::DataWriter_ptr w)
{
  ++w_enabled_;
  std::cout << "on_enabled " << w_enabled_ << to_str(w) << std::endl;
}

void TestObserver::on_enabled(DDS::DataReader_ptr r)
{
  ++r_enabled_;
  std::cout << "on_enabled " << r_enabled_ << to_str(r) << std::endl;
}

void TestObserver::on_deleted(DDS::DataWriter_ptr w)
{
  ++w_deleted_;
  std::cout << "on_deleted " << w_deleted_ << to_str(w) << std::endl;
}

void TestObserver::on_deleted(DDS::DataReader_ptr r)
{
  ++r_deleted_;
  std::cout << "on_deleted " << r_deleted_ << to_str(r) << std::endl;
}

void TestObserver::on_qos_changed(DDS::DataWriter_ptr w)
{
  ++w_qos_changed_;
  DDS::DataWriterQos qos;
  std::cout << "on_qos_changed " << w_qos_changed_ << to_str(w) << (w->get_qos(qos) == DDS::RETCODE_OK ? qos_str(qos) : "\n");
}

void TestObserver::on_qos_changed(DDS::DataReader_ptr r)
{
  ++r_qos_changed_;
  DDS::DataReaderQos qos;
  std::cout << "on_qos_changed " << r_qos_changed_ << to_str(r) << (r->get_qos(qos) == DDS::RETCODE_OK ? qos_str(qos) : "\n");
}

// ========== ========== ========== ========== ========== ========== ==========
// 2. Association: associated, disassociated

void TestObserver::on_associated(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId)
{
  ++w_associated_;
  std::cout << "on_associated " << w_associated_ << to_str(w) << " with reader " << to_str(readerId) << std::endl;
}

void TestObserver::on_associated(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId)
{
  ++r_associated_;
  std::cout << "on_associated " << r_associated_ << to_str(r) << " with writer " << to_str(writerId) << std::endl;
}

void TestObserver::on_disassociated(DDS::DataWriter_ptr w, const OpenDDS::DCPS::GUID_t& readerId)
{
  ++w_disassociated_;
  std::cout << "on_disassociated " << w_disassociated_ << to_str(w) << " from reader " << to_str(readerId) << std::endl;
}

void TestObserver::on_disassociated(DDS::DataReader_ptr r, const OpenDDS::DCPS::GUID_t& writerId)
{
  ++r_disassociated_;
  std::cout << "on_disassociated " << r_disassociated_ << to_str(r) << " from writer " << to_str(writerId) << std::endl;
}

#ifndef OPENDDS_RAPIDJSON
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {
  const char* to_json(const Observer::Sample&) { return ""; }
} }
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif

// ========== ========== ========== ========== ========== ========== ==========
// 3. Sample: sent, received, read, taken

void TestObserver::on_sample_sent(DDS::DataWriter_ptr w, const Sample& s)
{
  ++sent_;
  std::cout << "on_sample_sent " << sent_ << to_str(w) << ' ' << OpenDDS::DCPS::to_json(s) << std::endl;
}

void TestObserver::on_sample_received(DDS::DataReader_ptr r, const Sample& s)
{
  ++received_;
  std::cout << "on_sample_received " << received_ << to_str(r) << ' ' << OpenDDS::DCPS::to_json(s) << std::endl;
}

void TestObserver::on_sample_read(DDS::DataReader_ptr r, const Sample& s)
{
  ++read_;
  std::cout << "on_sample_read " << read_ << to_str(r) << ' ' << OpenDDS::DCPS::to_json(s) << std::endl;
}

void TestObserver::on_sample_taken(DDS::DataReader_ptr r, const Sample& s)
{
  ++taken_;
  std::cout << "on_sample_taken " << taken_ << to_str(r) << ' ' << OpenDDS::DCPS::to_json(s) << std::endl;
}

// ========== ========== ========== ========== ========== ========== ==========
bool TestObserver::check_w_g1_g2() const
{
  return w_enabled_ == n_WRITER && w_deleted_ == n_WRITER && w_qos_changed_ == n_WRITER
         && w_associated_ == n_ASSOCIATION && w_disassociated_ == n_ASSOCIATION;
}

bool TestObserver::check_r_all() const
{
  return r_enabled_ == n_READER && r_deleted_ == n_READER && r_qos_changed_ == n_READER
         && r_associated_ == n_ASSOCIATION && r_disassociated_ == n_ASSOCIATION
         && received_ >= n_RECEIVED && read_ == n_READ && taken_ >= n_TAKEN;
}

void TestObserver::show_observed(const std::string& txt) const
{
  std::cout << txt << ":\n"
    << "  w_enabled_:" << w_enabled_ << " w_deleted_:" << w_deleted_ << " w_qos_changed_:" << w_qos_changed_
    << "  w_associated_:" << w_associated_ << " w_disassociated_:" << w_disassociated_ << '\n'
    << "  r_enabled_:" << r_enabled_ << " r_deleted_:" << r_deleted_ << " r_qos_changed_:" << r_qos_changed_
    << "  r_associated_:" << r_associated_ << " r_disassociated_:" << r_disassociated_ << '\n'
    << "  sent_:" << sent_ << " received_:" << received_ << " read_:" << read_ << " taken_:" << taken_ << '\n';
}

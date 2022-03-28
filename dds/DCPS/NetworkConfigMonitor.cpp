/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigMonitor.h"
#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool NetworkInterfaceAddress::exclude_from_multicast(const char* configured_interface) const
{
  if (!can_multicast) {
    return true;
  }

  if (configured_interface && *configured_interface && name != configured_interface) {
    OPENDDS_STRING ci_semi(configured_interface);
    if (ci_semi.find_first_of(':') == OPENDDS_STRING::npos) {
      ci_semi += ':';
    }
    NetworkAddress as_addr(ci_semi.c_str());
    if (as_addr == NetworkAddress() || address != as_addr) {
      return true;
    }
  }

  const NetworkAddress sp_default = TheServiceParticipant->default_address();
  return sp_default != NetworkAddress() && address != sp_default;
}

NetworkConfigMonitor::NetworkConfigMonitor()
  : writer_(make_rch<InternalDataWriter<NetworkInterfaceAddress> >(true))
{}

void NetworkConfigMonitor::connect(RcHandle<InternalTopic<NetworkInterfaceAddress> > topic)
{
  topic->connect(writer_);
}

void NetworkConfigMonitor::disconnect(RcHandle<InternalTopic<NetworkInterfaceAddress> > topic)
{
  topic->disconnect(writer_);
}

void NetworkConfigMonitor::set(const List& list)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  for (List::const_iterator pos = list.begin(), limit = list.end(); pos != limit; ++pos) {
    List::const_iterator iter = std::find_if(list_.begin(), list_.end(), NetworkInterfaceAddressKeyEqual(*pos));
    if (iter != list_.end()) {
      if (*iter != *pos) {
        // Change.
        writer_->write(*pos);
      }
    } else {
      // New.
      writer_->write(*pos);
    }
  }

  for (List::const_iterator pos = list_.begin(), limit = list_.end(); pos != limit; ++pos) {
    List::const_iterator iter = std::find_if(list.begin(), list.end(), NetworkInterfaceAddressKeyEqual(*pos));
    if (iter == list.end()) {
      // Deleted.
      writer_->dispose(*pos);
    }
  }

  list_ = list;
}

void NetworkConfigMonitor::clear()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  for (List::const_iterator pos = list_.begin(), limit = list_.end(); pos != limit; ++pos) {
    writer_->dispose(*pos);
  }

  list_.clear();
}

void NetworkConfigMonitor::set(const NetworkInterfaceAddress& nia)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  List::iterator pos = std::find_if(list_.begin(), list_.end(), NetworkInterfaceAddressKeyEqual(nia));
  if (pos != list_.end()) {
    if (*pos != nia) {
      writer_->write(nia);
      *pos = nia;
    }
  } else {
    writer_->write(nia);
    list_.push_back(nia);
  }
}

void NetworkConfigMonitor::remove_interface(const OPENDDS_STRING& name)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  for (List::iterator pos = list_.begin(), limit = list_.end(); pos != limit;) {
    if (pos->name == name) {
      writer_->dispose(*pos);
      list_.erase(pos++);
    } else {
      ++pos;
    }
  }
}

void NetworkConfigMonitor::remove_address(const OPENDDS_STRING& name, const NetworkAddress& address)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  for (List::iterator pos = list_.begin(), limit = list_.end(); pos != limit;) {
    if (pos->name == name && pos->address == address) {
      writer_->dispose(*pos);
      list_.erase(pos++);
    } else {
      ++pos;
    }
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TcpInst.h"

#include <dds/DCPS/NetworkResource.h>

#include <ace/Configuration.h>

#include <iostream>
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TcpInst.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpInst::~TcpInst()
{
  DBG_ENTRY_LVL("TcpInst","~TcpInst",6);
}

OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TcpInst::new_impl(DDS::DomainId_t domain)
{
  return make_rch<TcpTransport>(rchandle_from(this), domain);
}

OPENDDS_STRING
OpenDDS::DCPS::TcpInst::dump_to_str(DDS::DomainId_t domain) const
{
  std::ostringstream os;
  os << TransportInst::dump_to_str(domain);

  os << formatNameForDump("local_address")                 << this->local_address() << std::endl;
  os << formatNameForDump("pub_address")                   << this->pub_address_str() << std::endl;
  os << formatNameForDump("enable_nagle_algorithm")        << (this->enable_nagle_algorithm() ? "true" : "false") << std::endl;
  os << formatNameForDump("conn_retry_initial_delay")      << this->conn_retry_initial_delay() << std::endl;
  os << formatNameForDump("conn_retry_backoff_multiplier") << this->conn_retry_backoff_multiplier() << std::endl;
  os << formatNameForDump("conn_retry_attempts")           << this->conn_retry_attempts() << std::endl;
  os << formatNameForDump("passive_reconnect_duration")    << this->passive_reconnect_duration() << std::endl;
  os << formatNameForDump("max_output_pause_period")       << this->max_output_pause_period() << std::endl;
  os << formatNameForDump("active_conn_timeout_period")    << this->active_conn_timeout_period() << std::endl;
  return OPENDDS_STRING(os.str());
}

size_t
OpenDDS::DCPS::TcpInst::populate_locator(OpenDDS::DCPS::TransportLocator& local_info,
                                         ConnectionInfoFlags,
                                         DDS::DomainId_t) const
{
  const std::string local_addr = local_address();
  const std::string locator_addr = get_locator_address();
  const std::string locator_a = locator_addr.empty() ? local_addr : locator_addr;
  if (!locator_a.empty()) {
    // Get the public address string from the inst (usually the local address)
    NetworkResource network_resource(locator_a);

    ACE_OutputCDR cdr;
    cdr << network_resource;
    const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
    char* buffer = const_cast<char*>(cdr.buffer()); // safe

    local_info.transport_type = "tcp";
    local_info.data = TransportBLOB(len, len,
                                    reinterpret_cast<CORBA::Octet*>(buffer));
    return 1;
  } else {
    return 0;
  }
}

void
OpenDDS::DCPS::TcpInst::pub_address_str(const String& pas)
{
  TheServiceParticipant->config_store()->set(config_key("PUB_ADDRESS").c_str(), pas);
}

OpenDDS::DCPS::String
OpenDDS::DCPS::TcpInst::pub_address_str() const
{
  return TheServiceParticipant->config_store()->get(config_key("PUB_ADDRESS").c_str(), "");
}

void
OpenDDS::DCPS::TcpInst::enable_nagle_algorithm(bool ena)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("ENABLE_NAGLE_ALGORITHM").c_str(), ena);
}

bool
OpenDDS::DCPS::TcpInst::enable_nagle_algorithm() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("ENABLE_NAGLE_ALGORITHM").c_str(), false);
}

void
OpenDDS::DCPS::TcpInst::conn_retry_initial_delay(int crid)
{
  TheServiceParticipant->config_store()->set_int32(config_key("CONN_RETRY_INITIAL_DELAY").c_str(), crid);
}

int
OpenDDS::DCPS::TcpInst::conn_retry_initial_delay() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("CONN_RETRY_INITIAL_DELAY").c_str(), 500);
}

void
OpenDDS::DCPS::TcpInst::conn_retry_backoff_multiplier(double crbm)
{
  TheServiceParticipant->config_store()->set_float64(config_key("CONN_RETRY_BACKOFF_MULTIPLIER").c_str(), crbm);
}

double
OpenDDS::DCPS::TcpInst::conn_retry_backoff_multiplier() const
{
  return TheServiceParticipant->config_store()->get_float64(config_key("CONN_RETRY_BACKOFF_MULTIPLIER").c_str(), 2.0);
}

void
OpenDDS::DCPS::TcpInst::conn_retry_attempts(int cra)
{
  TheServiceParticipant->config_store()->set_int32(config_key("CONN_RETRY_ATTEMPTS").c_str(), cra);
}

int
OpenDDS::DCPS::TcpInst::conn_retry_attempts() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("CONN_RETRY_ATTEMPTS").c_str(), 3);
}

void
OpenDDS::DCPS::TcpInst::max_output_pause_period(int mopp)
{
  TheServiceParticipant->config_store()->set_int32(config_key("MAX_OUTPUT_PAUSE_PERIOD").c_str(), mopp);
}

int
OpenDDS::DCPS::TcpInst::max_output_pause_period() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("MAX_OUTPUT_PAUSE_PERIOD").c_str(), -1);
}

void
OpenDDS::DCPS::TcpInst::passive_reconnect_duration(int prd)
{
  TheServiceParticipant->config_store()->set_int32(config_key("PASSIVE_RECONNECT_DURATION").c_str(), prd);
}

int
OpenDDS::DCPS::TcpInst::passive_reconnect_duration() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("PASSIVE_RECONNECT_DURATION").c_str(),
                                                          DEFAULT_PASSIVE_RECONNECT_DURATION);
}

void
OpenDDS::DCPS::TcpInst::active_conn_timeout_period(int actp)
{
  TheServiceParticipant->config_store()->set_int32(config_key("ACTIVE_CONN_TIMEOUT_PERIOD").c_str(), actp);
}

int
OpenDDS::DCPS::TcpInst::active_conn_timeout_period() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("ACTIVE_CONN_TIMEOUT_PERIOD").c_str(),
                                                          DEFAULT_ACTIVE_CONN_TIMEOUT_PERIOD);
}

void
OpenDDS::DCPS::TcpInst::local_address(const String& la)
{
  TheServiceParticipant->config_store()->set(config_key("LOCAL_ADDRESS").c_str(), la);
}

OpenDDS::DCPS::String
OpenDDS::DCPS::TcpInst::local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("LOCAL_ADDRESS").c_str(), "");
}

bool
OpenDDS::DCPS::TcpInst::set_locator_address(const ACE_INET_Addr& address)
{
  const ACE_INET_Addr local_addr = accept_address();
  const unsigned short port = address.get_port_number();

  if (local_addr.is_any()) {
    // As default, the acceptor will be listening on INADDR_ANY but advertise with the fully
    // qualified hostname and actual listening port number.
    const std::string hostname = get_fully_qualified_hostname();
    locator_address_ = hostname + ":" + to_dds_string(port);

    const ACE_INET_Addr addr = choose_single_coherent_address(locator_address_);
    if (addr == ACE_INET_Addr()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to resolve a local address using fully qualified hostname '%C'\n"),
                 hostname.c_str()));
      return false;
    }
  } else if (local_addr.get_port_number() == 0) {
    // Now we got the actual listening port. Update the port number in the configuration
    // if it's 0 originally.
    locator_address_ = local_address();
    set_port_in_addr_string(locator_address_, port);
  } else {
    locator_address_ = local_address();
  }

  return true;
}

ACE_INET_Addr
OpenDDS::DCPS::TcpInst::accept_address() const
{
  ACE_INET_Addr addr = choose_single_coherent_address(local_address(), false);
  // Override with DCPSDefaultAddress.
  if (addr == ACE_INET_Addr() &&
      TheServiceParticipant->default_address() != NetworkAddress::default_IPV4) {
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) TcpInst::accept_address overriding with DCPSDefaultAddress\n")), 2);
    addr = TheServiceParticipant->default_address().to_addr();
  }

  return addr;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_TCP_TCPINST_H
#define OPENDDS_DCPS_TRANSPORT_TCP_TCPINST_H

#include "Tcp_export.h"
#include "TcpTransport.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <ace/INET_Addr.h>
#include <ace/SString.h>

#include <string>

// Forward definition of a test-friendly class in the global name space
class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Tcp_Export TcpInst
  : public TransportInst {
public:

  static const int DEFAULT_PASSIVE_RECONNECT_DURATION = 2000;
  static const int DEFAULT_ACTIVE_CONN_TIMEOUT_PERIOD = 5000;

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str(DDS::DomainId_t domain) const;

  /// The address string provided to DCPSInfoRepo for connectors.
  /// This string is either from configuration file or defaults
  /// to the local address.
  ConfigValueRef<TcpInst, String> pub_address_str_;
  void pub_address_str(const String& pas);
  String pub_address_str() const;

  ConfigValue<TcpInst, bool> enable_nagle_algorithm_;
  void enable_nagle_algorithm(bool ena);
  bool enable_nagle_algorithm() const;

  /// The initial retry delay in milliseconds.
  /// The first connection retry will be when the loss of connection
  /// is detected.  The second try will be after this delay.
  /// The default is 500 miliseconds.
  ConfigValue<TcpInst, int> conn_retry_initial_delay_;
  void conn_retry_initial_delay(int crid);
  int conn_retry_initial_delay() const;

  /// The backoff multiplier for reconnection strategy.
  /// The third and so on reconnect will be this value * the previous delay.
  /// Hence with conn_retry_initial_delay=500 and conn_retry_backoff_multiplier=1.5
  /// the second reconnect attempt will be at 0.5 seconds after first retry connect
  /// fails; the third attempt will be 0.75 seconds after the second retry connect
  /// fails; the fourth attempt will be 1.125 seconds after the third retry connect
  /// fails.
  /// The default value is 2.0.
  ConfigValue<TcpInst, double> conn_retry_backoff_multiplier_;
  void conn_retry_backoff_multiplier(double crbm);
  double conn_retry_backoff_multiplier() const;

  /// Number of attempts to reconnect before giving up and calling
  /// on_publication_lost() and on_subscription_lost() callbacks.
  /// The default is 3.
  ConfigValue<TcpInst, int> conn_retry_attempts_;
  void conn_retry_attempts(int cra);
  int conn_retry_attempts() const;

  /// Maximum period (in milliseconds) of not being able to send queued
  /// messages. If there are samples queued and no output for longer
  /// than this period then the connection will be closed and on_*_lost()
  /// callbacks will be called. If the value is zero, the default, then
  /// this check will not be made.
  ConfigValue<TcpInst, int> max_output_pause_period_;
  void max_output_pause_period(int mopp);
  int max_output_pause_period() const;

  /// The time period in milliseconds for the acceptor side
  /// of a connection to wait for the connection to be reconnected.
  /// If not reconnected within this period then
  /// on_publication_lost() and on_subscription_lost() callbacks
  /// will be called.
  /// The default is 2 seconds (2000 millseconds).
  ConfigValue<TcpInst, int> passive_reconnect_duration_;
  void passive_reconnect_duration(int prd);
  int passive_reconnect_duration() const;

  /// The time period in milliseconds for the acceptor side
  /// of a connection to wait for the connection to be established.
  /// If not connected within this period then this link is removed
  /// from pending and any other links are attempted.
  /// The default is 5 seconds (5000 millseconds).
  ConfigValue<TcpInst, int> active_conn_timeout_period_;
  void active_conn_timeout_period(int actp);
  int active_conn_timeout_period() const;

  bool is_reliable() const { return true; }

  /// The address string used to configure the acceptor.
  /// This string is either from configuration file or default
  /// to hostname:port. The hostname is fully qualified hostname
  /// and the port is randomly picked by os.
  void local_address(const String& la);
  void local_address(const ACE_INET_Addr& addr)
  {
    local_address(LogAddr(addr).str());
  }
  void local_address(u_short port_number, const char* host_name)
  {
    local_address(String(host_name) + ":" + to_dds_string(port_number));
  }
  void local_address_set_port(u_short port_number) {
    String addr = local_address();
    set_port_in_addr_string(addr, port_number);
    local_address(addr);
  }
  String local_address() const;

  /// Describes the local endpoint to be used to accept
  /// passive connections.
  ACE_INET_Addr accept_address() const;

  bool set_locator_address(const ACE_INET_Addr& address);

  /// The public address is our publicly advertised address.
  /// Usually this is the same as the local address, but if
  /// a public address is explicitly specified, use that.
  std::string get_locator_address() const
  {
    const String pub_addr = pub_address_str();
    return (pub_addr == "") ? locator_address_ : pub_addr;
  }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info,
                                  ConnectionInfoFlags flags,
                                  DDS::DomainId_t domain) const;

private:
  friend class TcpType;
  friend class TcpTransport;
  friend class ::DDS_TEST;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit TcpInst(const OPENDDS_STRING& name);
  virtual ~TcpInst();

  TransportImpl_rch new_impl(DDS::DomainId_t domain);

  std::string locator_address_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TcpInst.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPINST_H */

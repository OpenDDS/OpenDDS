/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPINST_H
#define OPENDDS_TCPINST_H

#include "Tcp_export.h"
#include "TcpTransport.h"

#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include "ace/INET_Addr.h"
#include "ace/SString.h"

#include <string>

// Forward definition of a test-friendly class in the global name space
class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Tcp_Export TcpInst
  : public TransportInst {
public:
  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str() const;

  /// The address string provided to DCPSInfoRepo for connectors.
  /// This string is either from configuration file or defaults
  /// to the local address.
  std::string pub_address_str_;

  bool enable_nagle_algorithm_;

  /// The initial retry delay in milliseconds.
  /// The first connection retry will be when the loss of connection
  /// is detected.  The second try will be after this delay.
  /// The default is 500 miliseconds.
  int conn_retry_initial_delay_;

  /// The backoff multiplier for reconnection strategy.
  /// The third and so on reconnect will be this value * the previous delay.
  /// Hence with conn_retry_initial_delay=500 and conn_retry_backoff_multiplier=1.5
  /// the second reconnect attempt will be at 0.5 seconds after first retry connect
  /// fails; the third attempt will be 0.75 seconds after the second retry connect
  /// fails; the fourth attempt will be 1.125 seconds after the third retry connect
  /// fails.
  /// The default value is 2.0.
  double conn_retry_backoff_multiplier_;

  /// Number of attemps to reconnect before giving up and calling
  /// on_publication_lost() and on_subscription_lost() callbacks.
  /// The default is 3.
  int conn_retry_attempts_;

  /// Maximum period (in milliseconds) of not being able to send queued
  /// messages. If there are samples queued and no output for longer
  /// than this period then the connection will be closed and on_*_lost()
  /// callbacks will be called. If the value is zero, the default, then
  /// this check will not be made.
  int max_output_pause_period_;

  /// The time period in milliseconds for the acceptor side
  /// of a connection to wait for the connection to be reconnected.
  /// If not reconnected within this period then
  /// on_publication_lost() and on_subscription_lost() callbacks
  /// will be called.
  /// The default is 2 seconds (2000 millseconds).
  int passive_reconnect_duration_;

  /// The time period in milliseconds for the acceptor side
  /// of a connection to wait for the connection to be established.
  /// If not connected within this period then this link is removed
  /// from pending and any other links are attempted.
  /// The default is 5 seconds (5000 millseconds).
  int active_conn_timeout_period_;


  bool is_reliable() const { return true; }

  /// The public address is our publicly advertised address.
  /// Usually this is the same as the local address, but if
  /// a public address is explicitly specified, use that.
  const std::string& get_public_address() const {
    return (pub_address_str_ == "") ? local_address_str_ : pub_address_str_;
  }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const;

  OPENDDS_STRING local_address_string() const { return local_address_str_; }
  ACE_INET_Addr local_address() const { return local_address_; }
  void local_address(const char* str)
  {
    local_address_str_ = str;
    local_address_.set(str);
  }
  void local_address(u_short port_number, const char* host_name)
  {
    local_address_str_ = host_name;
    local_address_str_ += ":" + to_dds_string(port_number);
    local_address_.set(port_number, host_name);
  }
  void local_address_set_port(u_short port_number) {
    local_address_.set_port_number(port_number);
    set_port_in_addr_string(local_address_str_, port_number);
  }

private:
  friend class TcpType;
  friend class TcpTransport;
  friend class ::DDS_TEST;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit TcpInst(const OPENDDS_STRING& name);
  virtual ~TcpInst();

  TransportImpl_rch new_impl();

  /// Describes the local endpoint to be used to accept
  /// passive connections.
  ACE_INET_Addr local_address_;

  /// The address string used to configure the acceptor.
  /// This string is either from configuration file or default
  /// to hostname:port. The hostname is fully qualified hostname
  /// and the port is randomly picked by os.
  std::string local_address_str_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TcpInst.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPINST_H */

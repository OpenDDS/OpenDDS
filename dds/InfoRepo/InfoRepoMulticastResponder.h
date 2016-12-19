/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef INFOREPOMULTICASTRESPONDER_H
#define INFOREPOMULTICASTRESPONDER_H

#include /**/ "ace/pre.h"

#include "federator_export.h"
#include "tao/ORB.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/INET_Addr.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Reactor.h"
#include "ace/SString.h"

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {

/**
 * @class InfoRepoMulticastResponder
 *
 * @brief Event Handler that services multicast requests for IOR of a
 * bootstrappable service.
 *
 * This class uses the ACE_SOCK_Dgram_Mcast class and should be
 * registered with a reactor and should be initialized with the
 * ior of the  service to be multicasted.
 */
class OpenDDS_Federator_Export InfoRepoMulticastResponder : public ACE_Event_Handler {
public:
  InfoRepoMulticastResponder();

  virtual ~InfoRepoMulticastResponder();

  /// Initialization method.
  int init(
    CORBA::ORB_ptr orb,
    u_short port,
    const char *mcast_addr);

  /// Initialization method. Takes in "address:port" string as a
  /// parameter.
  int init(
    CORBA::ORB_ptr orb,
    const char *mcast_addr);

  /// Callback when input is received on the handle.
  virtual int handle_input(ACE_HANDLE n);

  /// Callback when a timeout has occurred.
  virtual int handle_timeout(const ACE_Time_Value &tv,
                             const void *arg);

  /// Returns the internal handle used to receive multicast.
  virtual ACE_HANDLE get_handle() const;

private:
  /// Factor common functionality from the two init functions.
  int common_init(
    CORBA::ORB_ptr orb);

  /// Are we initialized?
  bool initialized_;

  /// The ORB
  CORBA::ORB_var orb_;

  /// multicast endpoint of communication
  ACE_SOCK_Dgram_Mcast mcast_dgram_;

  /// multicast address
  ACE_INET_Addr mcast_addr_;

  /// address of response.
  ACE_INET_Addr response_addr_;

  /// socket for response to the multicast
  ACE_SOCK_Dgram response_;

  ACE_CString mcast_nic_;
};

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* INFOREPOMULTICASTRESPONDER_H */

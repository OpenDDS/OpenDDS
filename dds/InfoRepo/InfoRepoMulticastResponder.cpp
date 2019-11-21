/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "InfoRepoMulticastResponder.h"
#include "dds/DCPS/debug.h"

#include "tao/debug.h"
#include "tao/Object.h"
#include "tao/IORTable/IORTable.h"

#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {

InfoRepoMulticastResponder::InfoRepoMulticastResponder()
  : initialized_(false)
{
}

InfoRepoMulticastResponder::~InfoRepoMulticastResponder()
{
  if (
    this->initialized_ &&
    (this->mcast_dgram_.leave(this->mcast_addr_) == -1)) {
    ACE_ERROR((LM_ERROR, "%p\n", ACE_TEXT("~InfoRepoMulticastResponder()")));
  }
}

ACE_HANDLE
InfoRepoMulticastResponder::get_handle() const
{
  return this->mcast_dgram_.get_handle();
}

int
InfoRepoMulticastResponder::init(
  CORBA::ORB_ptr orb,
  u_short port,
  const char *mcast_addr)
{
  if (this->initialized_) {
    ACE_ERROR_RETURN((LM_ERROR, "InfoRepoMulticastResponder::init() already initialized\n"), -1);
  }

  if (this->mcast_addr_.set(port, mcast_addr) == -1)
    ACE_ERROR_RETURN((LM_ERROR, "InfoRepoMulticastResponder::init() %p\n", ACE_TEXT("set")), -1);

  return common_init(orb);
}

int
InfoRepoMulticastResponder::init(
  CORBA::ORB_ptr orb,
  const char *mcast_addr)
{
  if (this->initialized_) {
    ACE_ERROR_RETURN((LM_ERROR, "InfoRepoMulticastResponder::init() already initialized\n"), -1);
  }

  // Look for a '@' incase a nic is specified.
  const char* tmpnic = ACE_OS::strchr(mcast_addr, '@');

  CORBA::String_var actual_mcast_addr;
  CORBA::ULong length_addr;

  if (tmpnic != 0) {
    // i.e. a nic name has been specified
    length_addr = static_cast<CORBA::ULong>(tmpnic - mcast_addr + 1);
    actual_mcast_addr = CORBA::string_alloc(length_addr);

    ACE_OS::strncpy(actual_mcast_addr.inout(),
                    mcast_addr,
                    length_addr - 1);

    actual_mcast_addr[length_addr - 1] = '\0';

    /// Save for use later.
    this->mcast_nic_ = tmpnic + 1;

  } else {
    actual_mcast_addr =
      CORBA::string_alloc(static_cast<CORBA::ULong>(ACE_OS::strlen(mcast_addr)));

    actual_mcast_addr = mcast_addr;
  }

  if (this->mcast_addr_.set(actual_mcast_addr.in()) == -1)
    ACE_ERROR_RETURN((LM_ERROR,
                      "%p\n",
                      ACE_TEXT("set")),
                     -1);

  return common_init(orb);
}

int
InfoRepoMulticastResponder::common_init(
  CORBA::ORB_ptr orb)
{
  orb_ = CORBA::ORB::_duplicate(orb);

  if (this->response_addr_.set((u_short) 0) == -1)
    ACE_ERROR_RETURN((LM_ERROR,
                      "InfoRepoMulticastResponder::common_init() %p\n",
                      ACE_TEXT("set")),
                     -1);

  else if (this->response_.open(this->response_addr_) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "%p\n",
                      ACE_TEXT("set")),
                     -1);
  }

  // Use ACE_SOCK_Dgram_Mcast factory to subscribe to multicast group.
#ifdef ACE_HAS_MAC_OSX
  mcast_dgram_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                    ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif
  if (this->mcast_nic_.length() != 0) {
    if (this->mcast_dgram_.join(this->mcast_addr_,
                                1,
                                ACE_TEXT_CHAR_TO_TCHAR(this->mcast_nic_.c_str())) == -1)
      ACE_ERROR_RETURN((LM_ERROR, "InfoRepoMulticastResponder::common_init() %p\n",
                        ACE_TEXT("subscribe")), -1);

  } else {
    if (this->mcast_dgram_.join(this->mcast_addr_) == -1)
      ACE_ERROR_RETURN((LM_ERROR,
                        "InfoRepoMulticastResponder::common_init() %p\n",
                        ACE_TEXT("subscribe")),
                       -1);
  }

  this->initialized_ = true;
  return 0;
}

int
InfoRepoMulticastResponder::handle_timeout(const ACE_Time_Value &,
                                           const void *)
{
  return 0;
}

int
InfoRepoMulticastResponder::handle_input(ACE_HANDLE)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0)
    ACE_DEBUG((LM_DEBUG, "Entered InfoRepoMulticastResponder::handle_input\n"));

  // The length of the service name string that follows.
  CORBA::Short header;
  // Port to which to reply.
  ACE_UINT16 remote_port;
  // Name of the service for which the client is looking.
  char object_key[BUFSIZ];

  ACE_INET_Addr remote_addr;

  // Take a peek at the header to find out how long is the service
  // name string we should receive.
  ssize_t n = this->mcast_dgram_.recv(&header,
                                      sizeof(header),
                                      remote_addr,
                                      MSG_PEEK);

  if (n <= 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      "InfoRepoMulticastResponder::handle_input - peek %d\n",
                      n),
                     0);

  else if (ACE_NTOHS(header) <= 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      "InfoRepoMulticastResponder::handle_input() Header value < 1\n"),
                     0);

  // Receive full client multicast request.
  const int iovcnt = 3;
  iovec iov[iovcnt];

  iov[0].iov_base = (char *) &header;
  iov[0].iov_len  = sizeof(header);
  iov[1].iov_base = (char *) &remote_port;
  iov[1].iov_len  = sizeof(ACE_UINT16);
  iov[2].iov_base = (char *) object_key;
  iov[2].iov_len  = ACE_NTOHS(header);

  // Read the iovec.
  n = this->mcast_dgram_.recv(iov,
                              iovcnt,
                              remote_addr);

  if (n <= 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      "InfoRepoMulticastResponder::handle_input recv = %d\n",
                      n),
                     0);

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_TCHAR addr[64];
    remote_addr.addr_to_string(addr, sizeof(addr), 0);
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) Received multicast from %s.\n"
               "Service Name received : %C\n"
               "Port received : %u\n",
               addr,
               object_key,
               ACE_NTOHS(remote_port)));
  }

  // Grab the IOR table.
  CORBA::Object_var table_object =
    orb_->resolve_initial_references("IORTable");

  IORTable::Locator_var locator =
    IORTable::Locator::_narrow(table_object.in());

  if (CORBA::is_nil(locator.in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Nil IORTable\n")));

  }

  std::string ior;

  {
    CORBA::String_var ior_result;

    try {
      ior_result = locator->locate(object_key);

    } catch (const IORTable::NotFound&) {
      ACE_ERROR_RETURN((LM_ERROR,
                        "InfoRepoMulticastResponder::handle_input() Object key not found\n"),
                       0);
    }

    ior = ior_result;
  }

  // Reply to the multicast message.
  ACE_SOCK_Connector connector;
  ACE_INET_Addr peer_addr;
  ACE_SOCK_Stream stream;

  peer_addr.set(remote_addr);
  peer_addr.set_port_number(ACE_NTOHS(remote_port));

#if defined (ACE_HAS_IPV6)

  if (peer_addr.is_linklocal()) {
    // If this is one of our local linklocal interfaces this is not going
    // to work.
    // Creating a connection using such interface to the client listening
    // at the IPv6 ANY address is not going to work (I'm not quite sure why
    // but it probably has to do with the rather restrictive routing rules
    // for linklocal interfaces).
    // So we see if this is one of our local interfaces and if so create the
    // connection using the IPv6 loopback address instead.
    ACE_INET_Addr  peer_tmp(peer_addr);
    peer_tmp.set_port_number(static_cast<u_short>(0));
    ACE_INET_Addr* tmp = 0;
    size_t cnt = 0;
    int err = ACE::get_ip_interfaces(cnt, tmp);

    if (err == 0) {
      for (size_t i = 0; i < cnt; ++i) {
        if (peer_tmp == tmp[i]) {
          peer_addr.set(ACE_NTOHS(remote_port),
                        ACE_IPV6_LOCALHOST);
          break;
        }
      }

      delete[] tmp;
    }
  }

#endif /* ACE_HAS_IPV6 */

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_TCHAR addr[64];
    peer_addr.addr_to_string(addr, sizeof(addr), 0);
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) Replying to peer %s.\n",
               addr));
  }

  // Connect.
  if (connector.connect(stream, peer_addr) == -1)
    ACE_ERROR_RETURN((LM_ERROR, "InfoRepoMulticastResponder::connect failed\n"), 0);

  // Send the IOR back to the client.  (Send iovec, which contains ior
  // length as the first element, and ior itself as the second.)

  // Length of ior to be sent.
  CORBA::Short data_len = ACE_HTONS(static_cast<CORBA::Short>(ior.length()) + 1);

  // Vector to be sent.
  const int cnt = 2;
  iovec iovp[cnt];

  // The length of ior to be sent.
  iovp[0].iov_base = (char *) &data_len;
  iovp[0].iov_len  = sizeof(CORBA::Short);

  // The ior.
  iovp[1].iov_base = const_cast<char*>(ior.c_str());
  iovp[1].iov_len  = static_cast<u_long>(ior.length() + 1);

  ssize_t result = stream.sendv_n(iovp, cnt);
  // Close the stream.
  stream.close();

  // Check for error.
  if (result == -1)
    ACE_ERROR_RETURN((LM_ERROR, "InfoRepoMulticastResponder::send failed\n"), 0);

  if (OpenDDS::DCPS::DCPS_debug_level > 0)
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) InfoRepoMulticastResponder::handle_input() ior: <%C>\n"
               "sent to %C:%u.\n"
               "result from send = %d\n",
               ior.c_str(),
               peer_addr.get_host_name(),
               peer_addr.get_port_number(),
               result));

  return 0;
}

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

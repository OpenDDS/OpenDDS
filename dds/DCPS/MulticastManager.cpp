/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "MulticastManager.h"

#include "debug.h"
#include "LogAddr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool MulticastManager::process(InternalDataReader<NetworkInterfaceAddress>::SampleSequence& samples,
                               InternalSampleInfoSequence& infos,
                               const OPENDDS_STRING& multicast_interface,
                               ACE_Reactor* reactor,
                               ACE_Event_Handler* event_handler,
                               const NetworkAddress& multicast_group_address,
                               ACE_SOCK_Dgram_Mcast& multicast_socket
#ifdef ACE_HAS_IPV6
                               , const NetworkAddress& ipv6_multicast_group_address,
                               ACE_SOCK_Dgram_Mcast& ipv6_multicast_socket
#endif
                               )
{
  bool any_joined = false;

  for (size_t idx = 0; idx != samples.size(); ++idx) {
    NetworkInterfaceAddress& nia = samples[idx];
    const InternalSampleInfo& info = infos[idx];

    if (nia.name.empty()) {
      nia.name = multicast_interface;
    }

    switch (info.kind) {
    case ISIK_REGISTER:
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MulticastManager::process: unexpected REGISTER\n"));
      }
      break;
    case ISIK_SAMPLE: {
      if (nia.exclude_from_multicast(multicast_interface.c_str())) {
        continue;
      }

      if (joined_interfaces_.count(nia.name) == 0 && nia.is_ipv4()) {
        if (log_level >= LogLevel::Info) {
          ACE_DEBUG((LM_INFO,
                     "(%P|%t) MulticastManager::process: joining group %C on %C\n",
                     LogAddr(multicast_group_address).c_str(),
                     nia.name.empty() ? "all interfaces" : nia.name.c_str()));
        }

        if (0 == multicast_socket.join(multicast_group_address.to_addr(), 1, nia.name.empty() ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nia.name.c_str()))) {
          joined_interfaces_.insert(nia.name);
          any_joined = true;

          if (reactor) {
            if (reactor->register_handler(multicast_socket.get_handle(),
                                                   event_handler,
                                                   ACE_Event_Handler::READ_MASK) != 0) {
              if (log_level >= LogLevel::Error) {
                ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MulticastManager::process: failed to register multicast input handler\n"));
              }
            }
          } else {
            if (log_level >= LogLevel::Error) {
              ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MulticastManager::process: reactor is NULL\n"));
            }
          }
        } else {
          if (log_level >= LogLevel::Error) {
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) ERROR: MulticastManager::process: ACE_SOCK_Dgram_Mcast::join failed: %m\n"));
          }
        }
      }

#ifdef ACE_HAS_IPV6
      if (ipv6_joined_interfaces_.count(nia.name) == 0 && nia.is_ipv6()) {
        if (log_level >= LogLevel::Info) {
          ACE_DEBUG((LM_INFO,
                     "(%P|%t) MulticastManager::process: joining group %C on %C\n",
                     LogAddr(ipv6_multicast_group_address).c_str(),
                     nia.name.empty() ? "all interfaces" : nia.name.c_str()));
        }

        // Windows 7 has an issue with different threads concurrently calling join for ipv6
        static ACE_Thread_Mutex ipv6_static_lock;
        ACE_GUARD_RETURN(ACE_Thread_Mutex, g3, ipv6_static_lock, false);

        if (0 == ipv6_multicast_socket.join(ipv6_multicast_group_address.to_addr(), 1, nia.name.empty() ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nia.name.c_str()))) {
          ipv6_joined_interfaces_.insert(nia.name);
          any_joined = true;

          if (reactor) {
            if (reactor->register_handler(ipv6_multicast_socket.get_handle(),
                                          event_handler,
                                          ACE_Event_Handler::READ_MASK) != 0) {
              if (log_level >= LogLevel::Error) {
                ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MulticastManager::process: ipv6 failed to register multicast input handler\n"));
              }
            }
          } else {
            if (log_level >= LogLevel::Error) {
              ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MulticastManager::process: ipv6 reactor is NULL\n"));
            }
          }
        } else {
          if (log_level >= LogLevel::Error) {
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) ERROR: MulticastManager::process: ipv6 ACE_SOCK_Dgram_Mcast::join failed: %m\n"));
          }
        }
      }
#endif
    }
      break;
    case ISIK_UNREGISTER:
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MulticastManager::process: unexpected UNREGISTER\n"));
      }
      break;
    case ISIK_DISPOSE: {
      if (joined_interfaces_.count(nia.name) != 0 && !nia.is_ipv4()) {
        if (log_level >= LogLevel::Info) {
          ACE_DEBUG((LM_INFO,
                     "(%P|%t) MulticastManager::process: leaving group %C on %C\n",
                     LogAddr(multicast_group_address).c_str(),
                     nia.name.empty() ? "all interfaces" : nia.name.c_str()));
        }

        if (0 == multicast_socket.leave(multicast_group_address.to_addr(), nia.name.empty() ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nia.name.c_str()))) {
          joined_interfaces_.erase(nia.name);
        } else {
          if (log_level >= LogLevel::Error) {
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) ERROR: MulticastManager::process: ACE_SOCK_Dgram_Mcast::leave failed: %m\n"));
          }
        }
      }

#ifdef ACE_HAS_IPV6
      if (ipv6_joined_interfaces_.count(nia.name) != 0 && !nia.is_ipv6()) {
        if (log_level >= LogLevel::Info) {
          ACE_DEBUG((LM_INFO,
                     "(%P|%t) MulticastManager::process: leaving group %C on %C\n",
                     LogAddr(ipv6_multicast_group_address).c_str(),
                     nia.name.empty() ? "all interfaces" : nia.name.c_str()));
        }

        if (0 == ipv6_multicast_socket.leave(ipv6_multicast_group_address.to_addr(), nia.name.empty() ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nia.name.c_str()))) {
          ipv6_joined_interfaces_.erase(nia.name);
        } else {
          if (log_level >= LogLevel::Error) {
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) ERROR: MulticastManager::process: ACE_SOCK_Dgram_Mcast::leave failed: %m\n"));
          }
        }
      }
#endif
    }
      break;
    }
  }

  return any_joined;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

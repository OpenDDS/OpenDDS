/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#ifndef DDS_DCPS_HANDLE_REGISTRY_IMPL_H
#define DDS_DCPS_HANDLE_REGISTRY_IMPL_H

#include "HandleRegistry.h"

#include "dds/DCPS/GuidUtils.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class DdsSecurity_Export HandleRegistryImpl : public HandleRegistry {
public:
  ~HandleRegistryImpl();

  void insert_local_datareader_crypto_handle(const DCPS::RepoId& id,
                                             DDS::Security::DatareaderCryptoHandle handle,
                                             DDS::Security::EndpointSecurityAttributes attributes);
  DDS::Security::DatareaderCryptoHandle get_local_datareader_crypto_handle(const DCPS::RepoId& id) const;
  DDS::Security::EndpointSecurityAttributes get_local_datareader_security_attributes(const DCPS::RepoId& id) const;
  void remove_local_datareader_crypto_handle(const DCPS::RepoId& id);

  void insert_local_datawriter_crypto_handle(const DCPS::RepoId& id,
                                             DDS::Security::DatawriterCryptoHandle handle,
                                             DDS::Security::EndpointSecurityAttributes attributes);
  DDS::Security::DatawriterCryptoHandle get_local_datawriter_crypto_handle(const DCPS::RepoId& id) const;
  DDS::Security::EndpointSecurityAttributes get_local_datawriter_security_attributes(const DCPS::RepoId& id) const;
  void remove_local_datawriter_crypto_handle(const DCPS::RepoId& id);

  void insert_remote_participant_crypto_handle(const DCPS::RepoId& id,
                                               DDS::Security::ParticipantCryptoHandle handle);
  DDS::Security::ParticipantCryptoHandle get_remote_participant_crypto_handle(const DCPS::RepoId& id) const;
  void remove_remote_participant_crypto_handle(const DCPS::RepoId& id);

  void insert_remote_datareader_crypto_handle(const DCPS::RepoId& id,
                                              DDS::Security::DatareaderCryptoHandle handle,
                                              DDS::Security::EndpointSecurityAttributes attributes);
  DDS::Security::DatareaderCryptoHandle get_remote_datareader_crypto_handle(const DCPS::RepoId& id) const;
  DDS::Security::EndpointSecurityAttributes get_remote_datareader_security_attributes(const DCPS::RepoId& id) const;
  DatareaderCryptoHandleList get_all_remote_datareaders(const DCPS::RepoId& prefix) const;
  void remove_remote_datareader_crypto_handle(const DCPS::RepoId& id);

  void insert_remote_datawriter_crypto_handle(const DCPS::RepoId& id,
                                              DDS::Security::DatawriterCryptoHandle handle,
                                              DDS::Security::EndpointSecurityAttributes attributes);
  DDS::Security::DatawriterCryptoHandle get_remote_datawriter_crypto_handle(const DCPS::RepoId& id) const;
  DDS::Security::EndpointSecurityAttributes get_remote_datawriter_security_attributes(const DCPS::RepoId& id) const;
  DatawriterCryptoHandleList get_all_remote_datawriters(const DCPS::RepoId& prefix) const;
  void remove_remote_datawriter_crypto_handle(const DCPS::RepoId& id);

 private:
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DDS::Security::ParticipantCryptoHandle, DCPS::GUID_tKeyLessThan)
    ParticipantCryptoHandleMap;
  typedef std::pair<DDS::Security::DatareaderCryptoHandle, DDS::Security::EndpointSecurityAttributes> P1;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, P1, DCPS::GUID_tKeyLessThan)
    DatareaderCryptoHandleMap;
  typedef std::pair<DDS::Security::DatawriterCryptoHandle, DDS::Security::EndpointSecurityAttributes> P2;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, P2, DCPS::GUID_tKeyLessThan)
    DatawriterCryptoHandleMap;

  mutable ACE_Thread_Mutex mutex_;
  ParticipantCryptoHandleMap remote_participant_crypto_handles_;

  DatareaderCryptoHandleMap local_datareader_crypto_handles_;
  DatawriterCryptoHandleMap local_datawriter_crypto_handles_;

  DatareaderCryptoHandleMap remote_datareader_crypto_handles_;
  DatawriterCryptoHandleMap remote_datawriter_crypto_handles_;
};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

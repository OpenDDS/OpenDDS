/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#ifndef OPENDDS_DCPS_SECURITY_FRAMEWORK_HANDLEREGISTRY_H
#define OPENDDS_DCPS_SECURITY_FRAMEWORK_HANDLEREGISTRY_H

#include <dds/DCPS/dcps_export.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/RcObject.h>

#include <dds/DdsSecurityCoreC.h>

#include <ace/config.h>
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class OpenDDS_Dcps_Export HandleRegistry : public DCPS::RcObject {
public:
  typedef std::pair<DCPS::RepoId, DDS::Security::DatareaderCryptoHandle> RepoIdDrch;
  typedef OPENDDS_VECTOR(RepoIdDrch) DatareaderCryptoHandleList;
  typedef std::pair<DCPS::RepoId, DDS::Security::DatawriterCryptoHandle> RepoIdDwch;
  typedef OPENDDS_VECTOR(RepoIdDwch) DatawriterCryptoHandleList;

  HandleRegistry();
  ~HandleRegistry();

  const DDS::Security::EndpointSecurityAttributes& default_endpoint_security_attributes() const
  {
    return default_endpoint_security_attributes_;
  }

  void insert_local_datareader_crypto_handle(const DCPS::RepoId& id,
                                             DDS::Security::DatareaderCryptoHandle handle,
                                             const DDS::Security::EndpointSecurityAttributes& attributes);
  DDS::Security::DatareaderCryptoHandle get_local_datareader_crypto_handle(const DCPS::RepoId& id) const;
  const DDS::Security::EndpointSecurityAttributes& get_local_datareader_security_attributes(const DCPS::RepoId& id) const;
  void erase_local_datareader_crypto_handle(const DCPS::RepoId& id);

  void insert_local_datawriter_crypto_handle(const DCPS::RepoId& id,
                                             DDS::Security::DatawriterCryptoHandle handle,
                                             const DDS::Security::EndpointSecurityAttributes& attributes);
  DDS::Security::DatawriterCryptoHandle get_local_datawriter_crypto_handle(const DCPS::RepoId& id) const;
  const DDS::Security::EndpointSecurityAttributes& get_local_datawriter_security_attributes(const DCPS::RepoId& id) const;
  void erase_local_datawriter_crypto_handle(const DCPS::RepoId& id);

  void insert_remote_participant_crypto_handle(const DCPS::RepoId& id,
                                               DDS::Security::ParticipantCryptoHandle handle);
  DDS::Security::ParticipantCryptoHandle get_remote_participant_crypto_handle(const DCPS::RepoId& id) const;
  void erase_remote_participant_crypto_handle(const DCPS::RepoId& id);

  void insert_remote_participant_permissions_handle(const DCPS::RepoId& id,
                                                    DDS::Security::PermissionsHandle handle);
  DDS::Security::PermissionsHandle get_remote_participant_permissions_handle(const DCPS::RepoId& id) const;
  void erase_remote_participant_permissions_handle(const DCPS::RepoId& id);

  void insert_remote_datareader_crypto_handle(const DCPS::RepoId& id,
                                              DDS::Security::DatareaderCryptoHandle handle,
                                              const DDS::Security::EndpointSecurityAttributes& attributes);
  DDS::Security::DatareaderCryptoHandle get_remote_datareader_crypto_handle(const DCPS::RepoId& id) const;
  const DDS::Security::EndpointSecurityAttributes& get_remote_datareader_security_attributes(const DCPS::RepoId& id) const;
  DatareaderCryptoHandleList get_all_remote_datareaders(const DCPS::RepoId& prefix) const;
  void erase_remote_datareader_crypto_handle(const DCPS::RepoId& id);

  void insert_remote_datawriter_crypto_handle(const DCPS::RepoId& id,
                                              DDS::Security::DatawriterCryptoHandle handle,
                                              const DDS::Security::EndpointSecurityAttributes& attributes);
  DDS::Security::DatawriterCryptoHandle get_remote_datawriter_crypto_handle(const DCPS::RepoId& id) const;
  const DDS::Security::EndpointSecurityAttributes& get_remote_datawriter_security_attributes(const DCPS::RepoId& id) const;
  DatawriterCryptoHandleList get_all_remote_datawriters(const DCPS::RepoId& prefix) const;
  void erase_remote_datawriter_crypto_handle(const DCPS::RepoId& id);

private:
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DDS::Security::ParticipantCryptoHandle, DCPS::GUID_tKeyLessThan)
    ParticipantCryptoHandleMap;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DDS::Security::PermissionsHandle, DCPS::GUID_tKeyLessThan)
    PermissionsHandleMap;
  typedef std::pair<DDS::Security::DatareaderCryptoHandle, DDS::Security::EndpointSecurityAttributes> P1;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, P1, DCPS::GUID_tKeyLessThan)
    DatareaderCryptoHandleMap;
  typedef std::pair<DDS::Security::DatawriterCryptoHandle, DDS::Security::EndpointSecurityAttributes> P2;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, P2, DCPS::GUID_tKeyLessThan)
    DatawriterCryptoHandleMap;

  DDS::Security::EndpointSecurityAttributes default_endpoint_security_attributes_;

  mutable ACE_Thread_Mutex mutex_;
  ParticipantCryptoHandleMap remote_participant_crypto_handles_;
  PermissionsHandleMap remote_participant_permissions_handles_;

  DatareaderCryptoHandleMap local_datareader_crypto_handles_;
  DatawriterCryptoHandleMap local_datawriter_crypto_handles_;

  DatareaderCryptoHandleMap remote_datareader_crypto_handles_;
  DatawriterCryptoHandleMap remote_datawriter_crypto_handles_;
};

typedef DCPS::RcHandle<HandleRegistry> HandleRegistry_rch;


} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_SECURITY_FRAMEWORK_HANDLEREGISTRY_H
#endif // OPENDDS_SECURITY

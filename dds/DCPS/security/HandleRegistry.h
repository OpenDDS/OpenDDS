/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#ifndef DDS_DCPS_HANDLE_REGISTRY_H
#define DDS_DCPS_HANDLE_REGISTRY_H

#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class DdsSecurity_Export HandleRegistry {
public:
  virtual ~HandleRegistry() {}

  typedef std::pair<DCPS::RepoId, DDS::Security::DatareaderCryptoHandle> RepoIdDrch;
  typedef OPENDDS_VECTOR(RepoIdDrch) DatareaderCryptoHandleList;
  typedef std::pair<DCPS::RepoId, DDS::Security::DatawriterCryptoHandle> RepoIdDwch;
  typedef OPENDDS_VECTOR(RepoIdDwch) DatawriterCryptoHandleList;

  virtual void insert_local_datareader_crypto_handle(const DCPS::RepoId& id,
                                                     DDS::Security::DatareaderCryptoHandle handle,
                                                     DDS::Security::EndpointSecurityAttributes attributes) = 0;
  virtual DDS::Security::DatareaderCryptoHandle get_local_datareader_crypto_handle(const DCPS::RepoId& id) const = 0;
  virtual DDS::Security::EndpointSecurityAttributes get_local_datareader_security_attributes(const DCPS::RepoId& id) const = 0;
  virtual void remove_local_datareader_crypto_handle(const DCPS::RepoId& id) = 0;

  virtual void insert_local_datawriter_crypto_handle(const DCPS::RepoId& id,
                                                     DDS::Security::DatawriterCryptoHandle handle,
                                                     DDS::Security::EndpointSecurityAttributes attributes) = 0;
  virtual DDS::Security::DatawriterCryptoHandle get_local_datawriter_crypto_handle(const DCPS::RepoId& id) const = 0;
  virtual DDS::Security::EndpointSecurityAttributes get_local_datawriter_security_attributes(const DCPS::RepoId& id) const = 0;
  virtual void remove_local_datawriter_crypto_handle(const DCPS::RepoId& id) = 0;

  virtual void insert_remote_participant_crypto_handle(const DCPS::RepoId& id,
                                                       DDS::Security::ParticipantCryptoHandle handle) = 0;
  virtual DDS::Security::ParticipantCryptoHandle get_remote_participant_crypto_handle(const DCPS::RepoId& id) const = 0;
  virtual void remove_remote_participant_crypto_handle(const DCPS::RepoId& id) = 0;

  virtual void insert_remote_datareader_crypto_handle(const DCPS::RepoId& id,
                                                      DDS::Security::DatareaderCryptoHandle handle,
                                                      DDS::Security::EndpointSecurityAttributes attributes) = 0;
  virtual DDS::Security::DatareaderCryptoHandle get_remote_datareader_crypto_handle(const DCPS::RepoId& id) const = 0;
  virtual DDS::Security::EndpointSecurityAttributes get_remote_datareader_security_attributes(const DCPS::RepoId& id) const = 0;
  virtual DatareaderCryptoHandleList get_all_remote_datareaders(const DCPS::RepoId& prefix) const = 0;
  virtual void remove_remote_datareader_crypto_handle(const DCPS::RepoId& id) = 0;

  virtual void insert_remote_datawriter_crypto_handle(const DCPS::RepoId& id,
                                                      DDS::Security::DatawriterCryptoHandle handle,
                                                      DDS::Security::EndpointSecurityAttributes attributes) = 0;
  virtual DDS::Security::DatawriterCryptoHandle get_remote_datawriter_crypto_handle(const DCPS::RepoId& id) const = 0;
  virtual DDS::Security::EndpointSecurityAttributes get_remote_datawriter_security_attributes(const DCPS::RepoId& id) const = 0;
  virtual DatawriterCryptoHandleList get_all_remote_datawriters(const DCPS::RepoId& prefix) const = 0;
  virtual void remove_remote_datawriter_crypto_handle(const DCPS::RepoId& id) = 0;
};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_AUTHENTICATION_LOCALAUTHCREDENTIALDATA_H
#define OPENDDS_DCPS_SECURITY_AUTHENTICATION_LOCALAUTHCREDENTIALDATA_H

#include <string>

#include "dds/DCPS/security/SSL/Certificate.h"
#include "dds/DCPS/security/SSL/PrivateKey.h"
#include "dds/DCPS/security/SSL/DiffieHellman.h"
#include "dds/DCPS/security/SSL/Utils.h"

#include "dds/DCPS/RcObject.h"
#include "dds/DdsDcpsCoreC.h"
#include "dds/DdsSecurityCoreC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class OpenDDS_Security_Export CredentialHash
{
public:
  CredentialHash(const SSL::Certificate& cid, const SSL::DiffieHellman& dh, const DDS::OctetSeq& cpdata, const DDS::OctetSeq& cperm)
    : pubcert_(cid), dh_(dh), participant_topic_data_(cpdata), permissions_data_(cperm)
  {
  }

  int operator()(DDS::OctetSeq& dst) const;

private:
  const SSL::Certificate& pubcert_;
  const SSL::DiffieHellman& dh_;
  const DDS::OctetSeq& participant_topic_data_;
  const DDS::OctetSeq& permissions_data_;
};

class OpenDDS_Security_Export LocalAuthCredentialData : public DCPS::RcObject {
public:
  typedef DCPS::RcHandle<LocalAuthCredentialData> shared_ptr;

  LocalAuthCredentialData();

  virtual ~LocalAuthCredentialData();

  bool load_access_permissions(const DDS::Security::PermissionsCredentialToken& src,
                               DDS::Security::SecurityException& ex);

  bool load_credentials(const DDS::PropertySeq& props, DDS::Security::SecurityException& ex);

  const SSL::Certificate& get_ca_cert() const
  {
    return *ca_cert_;
  }

  const SSL::Certificate& get_participant_cert() const
  {
    return *participant_cert_;
  }

  const SSL::PrivateKey& get_participant_private_key() const
  {
    return *participant_pkey_;
  }

  const DDS::OctetSeq& get_access_permissions() const
  {
    return access_permissions_;
  }

  bool validate() const
  {
    if (!participant_cert_) {
      ACE_ERROR((LM_WARNING,
                "(%P|%t) LocalAuthCredentialData::validate(): WARNING: participant_cert_ is null,"
                " some of the security properties might be missing!\n"));
      return false;
    }
    if (!ca_cert_) {
      ACE_ERROR((LM_WARNING,
        "(%P|%t) LocalAuthCredentialData::validate(): WARNING: ca_cert_ is null,"
        " some of the security properties might be missing!\n"));
      return false;
    }
    return X509_V_OK == participant_cert_->validate(*ca_cert_);
  }

private:

  SSL::Certificate::unique_ptr ca_cert_;
  SSL::Certificate::unique_ptr participant_cert_;
  SSL::PrivateKey::unique_ptr participant_pkey_;
  DDS::OctetSeq access_permissions_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

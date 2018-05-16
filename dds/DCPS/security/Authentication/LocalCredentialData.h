/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef DDS_DCPS_AUTHENTICATION_LOCAL_CREDENTIAL_DATA_H
#define DDS_DCPS_AUTHENTICATION_LOCAL_CREDENTIAL_DATA_H

#include <string>

#include "dds/DCPS/security/SSL/Certificate.h"
#include "dds/DCPS/security/SSL/PrivateKey.h"
#include "dds/DCPS/security/SSL/DiffieHellman.h"
#include "dds/DCPS/security/SSL/Utils.h"

#include "dds/DdsDcpsCoreC.h"

namespace OpenDDS {
  namespace Security {

    class DdsSecurity_Export CredentialHash
    {
    public:
      CredentialHash(const SSL::Certificate& cid, const SSL::DiffieHellman& dh, const DDS::OctetSeq& cpdata, const DDS::OctetSeq& cperm) :
        pubcert_(cid), dh_(dh), participant_topic_data_(cpdata), permissions_data_(cperm)
      {

      }

      int operator()(DDS::OctetSeq& dst) const;

    private:
      const SSL::Certificate& pubcert_;
      const SSL::DiffieHellman& dh_;
      const DDS::OctetSeq& participant_topic_data_;
      const DDS::OctetSeq& permissions_data_;
    };

    class DdsSecurity_Export LocalAuthCredentialData
    {
    public:
      LocalAuthCredentialData(const DDS::PropertySeq& props);

      LocalAuthCredentialData();

      ~LocalAuthCredentialData();

      void load(const DDS::PropertySeq& props);

      SSL::Certificate& get_ca_cert()
      {
        return *ca_cert_;
      }

      SSL::Certificate& get_participant_cert()
      {
        return *participant_cert_;
      }

      SSL::PrivateKey& get_participant_private_key()
      {
        return *participant_pkey_;
      }

      const DDS::OctetSeq& get_access_permissions()
      {
        return access_permissions_;
      }

      bool validate()
      {
        if (!participant_cert_) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) CredentialHash::validate(): ERROR: participant_cert_ is null,")
            ACE_TEXT(" some of the security properties might be missing!\n")
          ));
          return false;
        }
        if (!ca_cert_) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) CredentialHash::validate(): ERROR: ca_cert_ is null,")
            ACE_TEXT(" some of the security properties might be missing!\n")
          ));
          return false;
        }
        return (X509_V_OK == participant_cert_->validate(*ca_cert_));
      }

    private:

      void load_permissions_file(const std::string& path);

	  void load_permissions_data(const std::string & path);

      SSL::Certificate::unique_ptr ca_cert_;
      SSL::Certificate::unique_ptr participant_cert_;
      SSL::PrivateKey::unique_ptr participant_pkey_;
      DDS::OctetSeq access_permissions_;
    };

  }
}

#endif

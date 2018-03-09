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

#include "dds/DdsDcpsCoreC.h"

namespace OpenDDS {
  namespace Security {

    class LocalAuthCredentialData
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

      bool validate()
      {
        return (X509_V_OK == participant_cert_->validate(*ca_cert_));
      }

    private:

      SSL::Certificate::unique_ptr ca_cert_;
      SSL::Certificate::unique_ptr participant_cert_;
      SSL::PrivateKey::unique_ptr participant_pkey_;
    };

  }
}

#endif

/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "LocalCredentialData.h"

namespace OpenDDS {
  namespace Security {

    LocalAuthCredentialData::LocalAuthCredentialData(const DDS::PropertySeq& props)
    {
      load(props);
    }

    LocalAuthCredentialData::LocalAuthCredentialData()
    {

    }

    LocalAuthCredentialData::~LocalAuthCredentialData()
    {

    }

    void LocalAuthCredentialData::load(const DDS::PropertySeq& props)
    {
      dhkey_.reset(new SSL::DiffieHellman());

      std::string name, value, pkey_uri, password;
      for (size_t i = 0; i < props.length(); ++i) {
        name = props[i].name;
        value = props[i].value;

        if (name == "dds.sec.auth.identity_ca") {
            ca_cert_.reset(new SSL::Certificate(value));

        } else if (name == "dds.sec.auth.private_key") {
            pkey_uri = value;

        } else if (name == "dds.sec.auth.identity_certificate") {
            participant_cert_.reset(new SSL::Certificate(value));

        } else if (name == "dds.sec.auth.password") {
            password = value;
        }
      }

      participant_pkey_.reset(new SSL::PrivateKey(pkey_uri, password));
    }
  }
}

/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "LocalCredentialData.h"

namespace OpenDDS {
  namespace Security {

    LocalCredentialData::LocalCredentialData(const DDS::PropertySeq& props)
    {
      load(props);
    }

    LocalCredentialData::LocalCredentialData()
    {

    }

    LocalCredentialData::~LocalCredentialData()
    {

    }

    void LocalCredentialData::load(const DDS::PropertySeq& props)
    {
      std::string name, value, pkey_uri, password;
      for (size_t i = 0; i < props.length(); ++i) {
        name = props[i].name;
        value = props[i].value;

        if (name == "dds.sec.auth.identity_ca") {
            ca_cert_ = SSL::Certificate(value);

        } else if (name == "dds.sec.auth.private_key") {
            pkey_uri = value;

        } else if (name == "dds.sec.auth.identity_certificate") {
            participant_cert_ = SSL::Certificate(value);

        } else if (name == "dds.sec.auth.password") {
            password = value;
        }
      }

      participant_pkey_ = SSL::PrivateKey(pkey_uri, password);
    }
  }
}

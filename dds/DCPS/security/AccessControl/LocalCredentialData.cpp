/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "LocalCredentialData.h"

namespace OpenDDS {
  namespace Security {

    LocalAccessCredentialData::LocalAccessCredentialData(const DDS::PropertySeq& props)
    {
      load(props);
    }

    LocalAccessCredentialData::LocalAccessCredentialData()
    {

    }

    LocalAccessCredentialData::~LocalAccessCredentialData()
    {

    }

    void LocalAccessCredentialData::load(const DDS::PropertySeq& props)
    {
      std::string name, value;
      for (size_t i = 0; i < props.length(); ++i) {
        name = props[i].name;
        value = props[i].value;

        if (name == "dds.sec.access.permissions_ca") {
            ca_cert_.reset(new SSL::Certificate(value));

        } else if (name == "dds.sec.access.governance") {
            governance_doc_.reset(new SSL::SignedDocument(value));

        } else if (name == "dds.sec.access.permissions") {
            permissions_doc_.reset(new SSL::SignedDocument(value));
        }
      }
    }

  }
}

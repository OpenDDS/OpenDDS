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
      bool permission = false,
           governance = false,
           ca = false;

      for (size_t i = 0; i < props.length(); ++i) {
        const std::string name = props[i].name.in();
        const std::string value = props[i].value.in();

        if (name == "dds.sec.access.permissions_ca") {
          ca_cert_.reset(new SSL::Certificate(value));
          ca = true;
        } else if (name == "dds.sec.access.governance") {
          governance_doc_.reset(new SSL::SignedDocument(value));
          governance = true;
        } else if (name == "dds.sec.access.permissions") {
          permissions_doc_.reset(new SSL::SignedDocument(value));
          permission = true;
        }
      }

      // If props did not have all 3 properties in it, set the missing properties to an empty string
      if (props.length() != 3) {
        if (!permission) {
          permissions_doc_.reset(new SSL::SignedDocument(""));
        }

        if (!governance) {
          governance_doc_.reset(new SSL::SignedDocument(""));
        }

        if (!ca) {
          ca_cert_.reset(new SSL::Certificate(""));
        }
      }
    }

  }
}

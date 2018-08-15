/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "LocalCredentialData.h"
#include "dds/DCPS/security/CommonUtilities.h"

namespace OpenDDS {
namespace Security {

    LocalAccessCredentialData::LocalAccessCredentialData()
    {

    }

    LocalAccessCredentialData::~LocalAccessCredentialData()
    {

    }

    CORBA::Boolean LocalAccessCredentialData::load(const DDS::PropertySeq& props,
                                                   ::DDS::Security::SecurityException& ex)
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

      if (! ca_cert_) {
        CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::load: CA certificate data not provided");
        return false;
      }

      if (! governance_doc_) {
        CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::load: Governance data not provided");
        return false;
      }

      if (! permissions_doc_) {
        CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::load: Permissions data not provided");
        return false;
      }

      return true;
    }

}
}

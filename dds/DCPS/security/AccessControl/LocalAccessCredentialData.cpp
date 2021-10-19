/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "LocalAccessCredentialData.h"

#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/security/framework/Properties.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

LocalAccessCredentialData::LocalAccessCredentialData()
{

}

LocalAccessCredentialData::~LocalAccessCredentialData()
{
}

bool LocalAccessCredentialData::load(const DDS::PropertySeq& props,
                                     DDS::Security::SecurityException& ex)
{
  for (unsigned int i = 0; i < props.length(); ++i) {
    const std::string name(props[i].name);
    const std::string value(props[i].value);

    if (name == DDS::Security::Properties::AccessPermissionsCA) {
      ca_cert_.reset(new SSL::Certificate(value));

    } else if (name == DDS::Security::Properties::AccessGovernance) {
      governance_doc_.reset(new SSL::SignedDocument(value));

    } else if (name == DDS::Security::Properties::AccessPermissions) {
      permissions_doc_.reset(new SSL::SignedDocument(value));
    }
  }

  if (! ca_cert_) {
    CommonUtilities::set_security_error(ex, -1, 0, "LocalAccessCredentialData::load: CA certificate data not provided");
    return false;
  }

  if (! governance_doc_) {
    CommonUtilities::set_security_error(ex, -1, 0, "LocalAccessCredentialData::load: Governance data not provided");
    return false;
  }

  if (! permissions_doc_) {
    CommonUtilities::set_security_error(ex, -1, 0, "LocalAccessCredentialData::load: Permissions data not provided");
    return false;
  }

  return true;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

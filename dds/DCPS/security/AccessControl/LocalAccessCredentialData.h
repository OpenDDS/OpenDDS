/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_ACCESSCONTROL_LOCALACCESSCREDENTIALDATA_H
#define OPENDDS_DCPS_SECURITY_ACCESSCONTROL_LOCALACCESSCREDENTIALDATA_H

#include <string>

#include "dds/DCPS/security/SSL/Certificate.h"
#include "dds/DCPS/security/SSL/SignedDocument.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/DdsDcpsCoreC.h"
#include "dds/DCPS/RcObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class LocalAccessCredentialData : public DCPS::RcObject {
public:
  typedef DCPS::RcHandle<LocalAccessCredentialData> shared_ptr;

  LocalAccessCredentialData();

  ~LocalAccessCredentialData();

  bool load(const DDS::PropertySeq& props, DDS::Security::SecurityException& ex);

  const SSL::Certificate& get_ca_cert() const
  {
    return *ca_cert_;
  }

  const SSL::SignedDocument& get_governance_doc() const
  {
    return *governance_doc_;
  }

  const SSL::SignedDocument& get_permissions_doc() const
  {
    return *permissions_doc_;
  }

private:

  SSL::Certificate::unique_ptr ca_cert_;
  SSL::SignedDocument::unique_ptr governance_doc_;
  SSL::SignedDocument::unique_ptr permissions_doc_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

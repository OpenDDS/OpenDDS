/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef DDS_DCPS_ACCESS_CONTROL_LOCAL_CREDENTIAL_DATA_H
#define DDS_DCPS_ACCESS_CONTROL_LOCAL_CREDENTIAL_DATA_H

#include <string>

#include "dds/DCPS/security/SSL/Certificate.h"
#include "dds/DCPS/security/SSL/SignedDocument.h"

#include "dds/DdsDcpsCoreC.h"

namespace OpenDDS {
namespace Security {

  class LocalAccessCredentialData
  {
  public:
    LocalAccessCredentialData(const DDS::PropertySeq& props);

    LocalAccessCredentialData();

    ~LocalAccessCredentialData();

    /**
     * @return 0 if load is successful.
     * @return 1 if certificate file could not be loaded
     * @return 2 if the governance file could not be loaded
     * @return 3 if the permissions file could not be loaded
     * @return 4 if the certificate filename not provided
     * @return 5 if the governance filename not provided
     * @return 6 if the permissions filename not provided
     * @return 7 if the certificate data not provided
     * @return 8 if the governance data not provided
     * @return 9 if the permissions data not provided
     */
    int load(const DDS::PropertySeq& props);

    const SSL::Certificate& get_ca_cert()
    {
      return *ca_cert_;
    }

    const SSL::SignedDocument& get_governance_doc()
    {
      return *governance_doc_;
    }

    const SSL::SignedDocument& get_permissions_doc()
    {
      return *permissions_doc_;
    }

  private:

    SSL::Certificate::unique_ptr ca_cert_;
    SSL::SignedDocument::unique_ptr governance_doc_;
    SSL::SignedDocument::unique_ptr permissions_doc_;

    std::string extract_file_name(const std::string& file_parm);
    ::CORBA::Boolean file_exists(const std::string& name);
  };

}
}

#endif

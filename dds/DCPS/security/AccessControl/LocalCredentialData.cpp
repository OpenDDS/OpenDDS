/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "LocalCredentialData.h"
#include "dds/DCPS/security/CommonUtilities.h"

namespace OpenDDS {
namespace Security {

    //LocalAccessCredentialData::LocalAccessCredentialData(const DDS::PropertySeq& props)
    //{
    //  load(props);
    //}

    LocalAccessCredentialData::LocalAccessCredentialData()
    {

    }

    LocalAccessCredentialData::~LocalAccessCredentialData()
    {

    }

    CORBA::Boolean LocalAccessCredentialData::load(const DDS::PropertySeq& props,
                                                   ::DDS::Security::SecurityException& ex)
    {
      const std::string file("file:");
      bool permission = false,
           governance = false,
           ca = false;

      for (size_t i = 0; i < props.length(); ++i) {
        const std::string name = props[i].name.in();
        const std::string value = props[i].value.in();

        if (name == "dds.sec.access.permissions_ca") {
          if (value.length() > 0) {
            if (value.find(file) != std::string::npos) {
              std::string fn = extract_file_name(value);

              if (!fn.empty()) {
                if (!file_exists(fn)) {
                  CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Certificate file could not be found");
                  return false;
                }
              }
            }
          }
          else {
            CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Certificate filename not provided");
            return false;
          }

          ca_cert_.reset(new SSL::Certificate(value));
          ca = true;
        } else if (name == "dds.sec.access.governance") {
          if (value.length() > 0) {
            if (value.find(file) != std::string::npos) {
              std::string fn = extract_file_name(value);

              if (!fn.empty()) {
                if (!file_exists(fn)) {
                  CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Governance file could not be found");
                  return false;
                }
              }
            }
          }
          else {
            CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Governance filename not provided");
            return false;
          }

          governance_doc_.reset(new SSL::SignedDocument(value));
          governance = true;
        } else if (name == "dds.sec.access.permissions") {
          if (value.length() > 0) {
            if (value.find(file) != std::string::npos) {
              std::string fn = extract_file_name(value);

              if (!fn.empty()) {
                if (!file_exists(fn)) {
                  CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Permissions file could not be found");
                  return false;
                }
              }
            }
          }
          else {
            CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Permissions filename not provided");
            return false;
          }

          permissions_doc_.reset(new SSL::SignedDocument(value));
          permission = true;
        }
      }

      // If props did not have all 3 properties in it, set the missing properties to an empty string
      if (props.length() != 3) {
        if (!permission) {
          CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Certificate data not provided");
          return false;
        }

        if (!governance) {
          CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Governance data not provided");
          return false;
        }

        if (!ca) {
          CommonUtilities::set_security_error(ex, -1, 0, "AccessControlBuiltInImpl::validate_local_permissions: Permissions data not provided");
          return false;
        }
      }

      return true;
    }

    std::string LocalAccessCredentialData::extract_file_name(const std::string & file_parm)
    {
      std::string del = ":";
      u_long pos = file_parm.find_last_of(del);
      if ((pos > 0UL) && (pos != file_parm.length() - 1)) {
        return file_parm.substr(pos + 1);
      }
      else {
        return std::string("");
      }
    }

    ::CORBA::Boolean LocalAccessCredentialData::file_exists(const std::string & name)
    {
      struct stat buffer;
      return (stat(name.c_str(), &buffer) == 0);
    }

}
}

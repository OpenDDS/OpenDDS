/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
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

    int LocalAccessCredentialData::load(const DDS::PropertySeq& props)
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
                  return 1;
                }
              }
            }
          }
          else {
            return 4;
          }

          ca_cert_.reset(new SSL::Certificate(value));
          ca = true;
        } else if (name == "dds.sec.access.governance") {
          if (value.length() > 0) {
            if (value.find(file) != std::string::npos) {
              std::string fn = extract_file_name(value);

              if (!fn.empty()) {
                if (!file_exists(fn)) {
                  return 2;
                }
              }
            }
          }
          else {
            return 5;
          }

          governance_doc_.reset(new SSL::SignedDocument(value));
          governance = true;
        } else if (name == "dds.sec.access.permissions") {
          if (value.length() > 0) {
            if (value.find(file) != std::string::npos) {
              std::string fn = extract_file_name(value);

              if (!fn.empty()) {
                if (!file_exists(fn)) {
                  return 3;
                }
              }
            }
          }
          else {
            return 6;
          }

          permissions_doc_.reset(new SSL::SignedDocument(value));
          permission = true;
        }
      }

      // If props did not have all 3 properties in it, set the missing properties to an empty string
      if (props.length() != 3) {
        if (!permission) {
          return 7;
        }

        if (!governance) {
          return 8;
        }

        if (!ca) {
          return 9;
        }
      }

      return 0;
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

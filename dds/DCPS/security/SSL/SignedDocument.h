/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_SIGNEDDOCUMENT_H
#define OPENDDS_DCPS_SECURITY_SSL_SIGNEDDOCUMENT_H

#include "Certificate.h"

#include <dds/DCPS/security/OpenDDS_Security_Export.h>
#include <dds/DCPS/unique_ptr.h>
#include <dds/DdsSecurityCoreC.h>

#include <openssl/pkcs7.h>

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

class OpenDDS_Security_Export SignedDocument {
public:
  explicit SignedDocument(const DDS::OctetSeq& src);

  SignedDocument();
  virtual ~SignedDocument();

  bool load(const std::string& uri, DDS::Security::SecurityException& ex);
  bool verify(const Certificate& ca);

  const DDS::OctetSeq& original() const { return original_; }
  void content(const std::string& value) { content_ = value; }
  const std::string& content() const { return content_; }
  bool verified() const { return verified_; }
  const std::string& filename() const { return filename_; }

  bool operator==(const SignedDocument& other) const;

private:
  void load_file(const std::string& path);

  DDS::OctetSeq original_;
  std::string content_;
  bool verified_;
  std::string filename_;
};

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

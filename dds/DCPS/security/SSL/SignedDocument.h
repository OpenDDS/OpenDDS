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
  typedef DCPS::unique_ptr<SignedDocument> unique_ptr;

  friend OpenDDS_Security_Export bool operator==(
    const SignedDocument& lhs, const SignedDocument& rhs);

  explicit SignedDocument(const std::string& uri);

  explicit SignedDocument(const DDS::OctetSeq& src);

  SignedDocument(const SignedDocument& rhs);

  SignedDocument();

  virtual ~SignedDocument();

  SignedDocument& operator=(const SignedDocument& rhs);

  bool load(const std::string& uri, DDS::Security::SecurityException& ex);

  void get_original(std::string& dst) const;

  const DDS::OctetSeq& get_original() const
  {
    return original_;
  }

  const std::string& get_verifiable() const
  {
    return verifiable_;
  }

  bool get_original_minus_smime(std::string& dst) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int verify_signature(const Certificate& ca) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int serialize(DDS::OctetSeq& dst) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int deserialize(const DDS::OctetSeq& src);

  /**
   * @return int 0 on success; 1 on failure.
   */
  int deserialize(const std::string& src);

  const std::string filename() const
  {
    return filename_;
  }

private:

  bool loaded()
  {
    return doc_ && original_.length() && verifiable_.length();
  }

  /**
   * @return int 0 on success; 1 on failure.
   *
   * @param from BIO containing data populated by a call to SMIME_read_PKCS7.
   */
  int cache_verifiable(BIO* from);

  PKCS7* PKCS7_from_SMIME_file(const std::string& path);

  PKCS7* PKCS7_from_data(const DDS::OctetSeq& s_mime_data);

  PKCS7* doc_;
  DDS::OctetSeq original_;
  std::string verifiable_;
  std::string filename_;
};

OpenDDS_Security_Export bool operator==(
  const SignedDocument& lhs, const SignedDocument& rhs);

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

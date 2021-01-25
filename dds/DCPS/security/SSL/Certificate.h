/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_CERTIFICATE_H
#define OPENDDS_DCPS_SECURITY_SSL_CERTIFICATE_H

#include <dds/DCPS/security/OpenDDS_Security_Export.h>
#include <dds/DCPS/unique_ptr.h>

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsSecurityCoreC.h>

#include <openssl/x509.h>

#include <string>
#include <vector>
#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

class verify_signature_impl;

class OpenDDS_Security_Export Certificate {
public:
  friend class verify_signature_impl;

  typedef DCPS::unique_ptr<Certificate> unique_ptr;

  friend OpenDDS_Security_Export std::ostream& operator<<(std::ostream&,
                                                     const Certificate&);

  friend OpenDDS_Security_Export bool operator==(const Certificate& lhs,
                                            const Certificate& rhs);

  explicit Certificate(const std::string& uri, const std::string& password = "");

  explicit Certificate(const DDS::OctetSeq& src);

  Certificate(const Certificate& other);

  Certificate();

  virtual ~Certificate();

  Certificate& operator=(const Certificate& rhs);

  bool load(DDS::Security::SecurityException& ex,
            const std::string& uri,
            const std::string& password = "");

  /**
   * @return int 0 on success; 1 on failure.
   */
  int validate(const Certificate& ca, unsigned long int flags = 0u) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int verify_signature(
                       const DDS::OctetSeq& src,
                       const std::vector<const DDS::OctetSeq*>& expected_contents) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int subject_name_to_str(std::string& dst,
                          unsigned long flags = XN_FLAG_ONELINE) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int subject_name_digest(std::vector<CORBA::Octet>& dst) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int serialize(DDS::OctetSeq& dst) const;

  /**
   * @return int 0 on success; 1 on failure.
   */
  int deserialize(const DDS::OctetSeq& src);

  const DDS::OctetSeq& original_bytes() const { return original_bytes_; }

  const char* dsign_algo() const { return dsign_algo_.c_str(); }

  const char* keypair_algo() const;

 private:

  bool loaded()
  {
    return (x_ != NULL) &&
      (0 < original_bytes_.length());
  }

  /**
   * @return int 0 on success; 1 on failure.
   */
  int cache_dsign_algo();


  void load_cert_bytes(const std::string& path);

  void load_cert_data_bytes(const std::string& data);

  static X509* x509_from_pem(const std::string& path,
                             const std::string& password = "");
  static X509* x509_from_pem(const DDS::OctetSeq& bytes,
                             const std::string& password = "");

  X509* x_;
  DDS::OctetSeq original_bytes_;
  std::string dsign_algo_;
};

OpenDDS_Security_Export std::ostream& operator<<(std::ostream&, const Certificate&);

OpenDDS_Security_Export bool operator==(const Certificate& lhs,
                                   const Certificate& rhs);
}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

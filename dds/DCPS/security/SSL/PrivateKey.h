/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_PRIVATEKEY_H
#define OPENDDS_DCPS_SECURITY_SSL_PRIVATEKEY_H

#include <dds/DCPS/security/OpenDDS_Security_Export.h>
#include <dds/DCPS/unique_ptr.h>

#include <dds/DdsDcpsCoreC.h>

#include <openssl/evp.h>

#include <string>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

class OpenDDS_Security_Export PrivateKey
{
public:
  typedef DCPS::unique_ptr<PrivateKey> unique_ptr;

  friend OpenDDS_Security_Export bool operator==(const PrivateKey& lhs,
                                            const PrivateKey& rhs);

  explicit PrivateKey(const std::string& uri, const std::string& password = "");

  PrivateKey();

  virtual ~PrivateKey();

  void load(const std::string& uri, const std::string& password = "");

  int sign(const std::vector<const DDS::OctetSeq*>& src,
           DDS::OctetSeq& dst) const;

private:
  PrivateKey(const PrivateKey&);
  PrivateKey& operator=(const PrivateKey&);

  static EVP_PKEY* EVP_PKEY_from_pem(const std::string& path,
                                     const std::string& password = "");

  static EVP_PKEY* EVP_PKEY_from_pem_data(const std::string& data,
                                          const std::string& password);

  EVP_PKEY* k_;
};

OpenDDS_Security_Export bool operator==(const PrivateKey& lhs,
                                   const PrivateKey& rhs);

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

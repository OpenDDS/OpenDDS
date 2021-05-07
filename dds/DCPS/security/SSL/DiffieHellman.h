/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_DIFFIEHELLMAN_H
#define OPENDDS_DCPS_SECURITY_SSL_DIFFIEHELLMAN_H

#include <dds/DCPS/security/OpenDDS_Security_Export.h>
#include <dds/DCPS/unique_ptr.h>

#include "dds/DdsDcpsCoreC.h"

#include <openssl/evp.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

const char DH_2048_MODP_256_PRIME_STR[] = "DH+MODP-2048-256";
const char ECDH_PRIME_256_V1_CEUM_STR[] = "ECDH+prime256v1-CEUM";

class OpenDDS_Security_Export DHAlgorithm {
public:
  typedef DCPS::unique_ptr<DHAlgorithm> unique_ptr;

  DHAlgorithm() : k_(0) {}

  virtual ~DHAlgorithm();

  virtual int init() = 0;
  virtual int pub_key(DDS::OctetSeq& dst) = 0;

  virtual int gen_shared_secret(const DDS::OctetSeq& pub_key)
  {
    return compute_shared_secret(pub_key) || hash_shared_secret();
  }

  virtual const DDS::OctetSeq& get_shared_secret() const
  {
    return shared_secret_;
  }

  virtual bool cmp_shared_secret(const DHAlgorithm& other) const;
  virtual const char* kagree_algo() const = 0;

 protected:
  virtual int compute_shared_secret(const DDS::OctetSeq& pub_key) = 0;
  int hash_shared_secret();

  EVP_PKEY* k_;
  DDS::OctetSeq shared_secret_;
};

class OpenDDS_Security_Export DH_2048_MODP_256_PRIME : public DHAlgorithm {
public:
  DH_2048_MODP_256_PRIME();
  ~DH_2048_MODP_256_PRIME();

  /**
   * @return int 0 on success; 1 on failure.
   */
  int init();

  /**
   * @return int 0 on success; 1 on failure.
   */
  int pub_key(DDS::OctetSeq& dst);

  /**
   * @return int 0 on success; 1 on failure.
   */
  int compute_shared_secret(const DDS::OctetSeq& pub_key);

  const char* kagree_algo() const { return DH_2048_MODP_256_PRIME_STR; }
};

class OpenDDS_Security_Export ECDH_PRIME_256_V1_CEUM : public DHAlgorithm {
public:
  ECDH_PRIME_256_V1_CEUM();
  ~ECDH_PRIME_256_V1_CEUM();

  /**
   * @return int 0 on success; 1 on failure.
   */
  int init();

  /**
   * @return int 0 on success; 1 on failure.
   */
  int pub_key(DDS::OctetSeq& dst);

  /**
   * @return int 0 on success; 1 on failure.
   */
  int compute_shared_secret(const DDS::OctetSeq& pub_key);

  const char* kagree_algo() const { return ECDH_PRIME_256_V1_CEUM_STR; }
};

class OpenDDS_Security_Export DiffieHellman {
public:
  typedef DCPS::unique_ptr<DiffieHellman> unique_ptr;

  static DiffieHellman* factory(const DDS::OctetSeq& kagree_algo);

  explicit DiffieHellman(DHAlgorithm* algorithm) : algo_(algorithm) {}

  ~DiffieHellman() {}

  void load()
  {
    if (algo_) algo_->init();
  }

  /**
   * @return int 0 on success; 1 on failure.
   */
  int pub_key(DDS::OctetSeq& dst) { return algo_->pub_key(dst); }

  /**
   * @return int 0 on success; 1 on failure.
   */
  int gen_shared_secret(const DDS::OctetSeq& pub_key)
  {
    return algo_->gen_shared_secret(pub_key);
  }

  const DDS::OctetSeq& get_shared_secret()
  {
    return algo_->get_shared_secret();
  }

  bool cmp_shared_secret(const DiffieHellman& other)
  {
    return algo_->cmp_shared_secret(*other.algo_);
  }

  const char* kagree_algo() const { return algo_->kagree_algo(); }

 private:
  DHAlgorithm::unique_ptr algo_;
};

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

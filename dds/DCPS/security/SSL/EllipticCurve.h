/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_ELLIPTIC_CURVE_H
#define OPENDDS_SECURITY_SSL_ELLIPTIC_CURVE_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/unique_ptr.h"
#include "dds/DdsDcpsCoreC.h"
#include <openssl/ossl_typ.h>

namespace OpenDDS {
namespace Security {
namespace SSL {

  class DdsSecurity_Export ECAlgorithm
  {
  public:
    typedef DCPS::unique_ptr<ECAlgorithm> unique_ptr;

    ECAlgorithm() : 
      k_(NULL)
    {

    }

    virtual ~ECAlgorithm();

    virtual int init() = 0;
    virtual int pub_key(DDS::OctetSeq& dst) = 0;
    virtual int gen_shared_secret(const DDS::OctetSeq& pub_key)
    {
      int err = compute_shared_secret(pub_key) || hash_shared_secret();
      return err;
    }
    virtual const DDS::OctetSeq& get_shared_secret() const
    {
      return shared_secret_;
    }
    virtual bool cmp_shared_secret(const ECAlgorithm& other) const;
    virtual const char* kagree_algo() = 0;

  protected:
    virtual int compute_shared_secret(const DDS::OctetSeq& pub_key) = 0;
    int hash_shared_secret();

    EVP_PKEY* k_;
    DDS::OctetSeq shared_secret_;
  };

  class DdsSecurity_Export EC_PRIME_256_V1_CEUM : public ECAlgorithm
  {
  public:
    EC_PRIME_256_V1_CEUM();
    ~EC_PRIME_256_V1_CEUM();

    int init();

    int pub_key(DDS::OctetSeq& dst);

    int compute_shared_secret(const DDS::OctetSeq& pub_key);

    const char* kagree_algo() {
      return "EC+prime256v1-CEUM";
    }
  };

  class DdsSecurity_Export EllipticCurve
  {
  public:
    typedef DCPS::unique_ptr<EllipticCurve> unique_ptr;

    static EllipticCurve* factory(const DDS::OctetSeq& kagree_algo);

    EllipticCurve(ECAlgorithm* algorithm) :
      algo_(algorithm)
    {

    }

    ~EllipticCurve()
    {

    }

    void load() {
      if (algo_) algo_->init();
    }

    int pub_key(DDS::OctetSeq& dst) {
      return algo_->pub_key(dst);
    }

    int gen_shared_secret(const DDS::OctetSeq& pub_key) {
      return algo_->gen_shared_secret(pub_key);
    }

    const DDS::OctetSeq& get_shared_secret() {
      return algo_->get_shared_secret();
    }

    bool cmp_shared_secret(const EllipticCurve& other) {
      return algo_->cmp_shared_secret(*other.algo_);
    }

    const char* kagree_algo() const {
      return algo_->kagree_algo();
    }

  private:

    ECAlgorithm::unique_ptr algo_;
  };
}
}
}

#endif

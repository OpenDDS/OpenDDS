/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_DIFFIE_HELLMAN_H
#define OPENDDS_SECURITY_SSL_DIFFIE_HELLMAN_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/unique_ptr.h"
#include "dds/DdsDcpsCoreC.h"
#include <openssl/evp.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class DdsSecurity_Export DHAlgorithm
      {
      public:
        typedef DCPS::unique_ptr<DHAlgorithm> unique_ptr;

        DHAlgorithm() : k_(NULL)
        {

        }

        virtual ~DHAlgorithm()
        {
          if (k_) EVP_PKEY_free(k_);
        }

        virtual DHAlgorithm& operator= (const DHAlgorithm& rhs)
        {
          if (this != &rhs) {
            if (rhs.k_) {
              k_ = rhs.k_;
              EVP_PKEY_up_ref(k_);

            } else {
              k_ = NULL;
            }
          }
          return *this;
        }

        virtual int init() = 0;
        virtual int pub_key(DDS::OctetSeq& dst) = 0;
        virtual int gen_shared_secret(const DDS::OctetSeq& pub_key) = 0;
        virtual const DDS::OctetSeq& get_shared_secret() const
        {
          return shared_secret_;
        }
        virtual bool cmp_shared_secret(const DHAlgorithm& other) = 0;
        virtual const char* kagree_algo() = 0;

      protected:
        EVP_PKEY* k_;
        DDS::OctetSeq shared_secret_;
      };

      class DdsSecurity_Export DH_2048_MODP_256_PRIME : public DHAlgorithm
      {
      public:
        DH_2048_MODP_256_PRIME();
        ~DH_2048_MODP_256_PRIME();

        int init();

        int pub_key(DDS::OctetSeq& dst);

        int gen_shared_secret(const DDS::OctetSeq& pub_key);

        bool cmp_shared_secret(const DHAlgorithm& other);

        const char* kagree_algo() {
          return "DH+MODP-2048-256";
        }

      };


      class DdsSecurity_Export DiffieHellman
      {
      public:

        typedef DCPS::unique_ptr<DiffieHellman> unique_ptr;

        DiffieHellman() : algo_(new DH_2048_MODP_256_PRIME())
        {

        }

        template <typename Algorithm>
        DiffieHellman() : algo_(new Algorithm)
        {

        }

        ~DiffieHellman()
        {

        }

        void load()
        {
          if (algo_) algo_->init();
        }

        int pub_key(DDS::OctetSeq& dst)
        {
          return algo_->pub_key(dst);
        }

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
          return algo_->cmp_shared_secret(* other.algo_);
        }

        const char* kagree_algo()
        {
          return algo_->kagree_algo();
        }

      private:

        DHAlgorithm::unique_ptr algo_;
      };

    }
  }
}

#endif

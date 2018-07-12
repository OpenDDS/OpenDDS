/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "PrivateKey.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "Err.h"
#include <openssl/pem.h>
#include "../OpenSSL_legacy.h"  // Must come after all other OpenSSL includes
#include <cstring>
#include <sstream>

namespace OpenDDS {
namespace Security {
namespace SSL {

  PrivateKey::PrivateKey(const std::string& uri, const std::string password)
    : k_(NULL)
  {
    load(uri, password);
  }

  PrivateKey::PrivateKey() : k_(NULL) {}

  PrivateKey::~PrivateKey()
  {
    if (k_) EVP_PKEY_free(k_);
  }

  PrivateKey& PrivateKey::operator=(const PrivateKey& rhs)
  {
    if (this != &rhs) {
      if (rhs.k_) {
        k_ = rhs.k_;
#ifndef OPENSSL_V_1_0
        EVP_PKEY_up_ref(k_);
#endif

      } else {
        k_ = NULL;
      }
    }
    return *this;
  }

  void PrivateKey::load(const std::string& uri, const std::string& password)
  {
    using namespace CommonUtilities;

    if (k_) return;

    std::string uri_info;
    URI_SCHEME s = extract_uri_info(uri, uri_info);

    switch (s) {
      case URI_FILE:
        k_ = EVP_PKEY_from_pem(uri_info, password);
        break;

      case URI_DATA:
        k_ = EVP_PKEY_from_pem_data(uri_info, password);
        break;

      case URI_PKCS11:
      case URI_UNKNOWN:
      default:
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) SSL::PrivateKey::load: WARNING: Unsupported URI scheme in cert path '%C'\n"),
                   uri.c_str()));

        break;
    }
  }

  //      class sign_rsassa_pss_mgf1_sha256_impl
  class sign_implementation
  {
   public:
    sign_implementation(EVP_PKEY* pkey)
      : private_key(pkey), md_ctx(NULL), pkey_ctx(NULL)
    {
    }
    ~sign_implementation()
    {
      if (md_ctx) EVP_MD_CTX_free(md_ctx);
    }

    int operator()(const std::vector<const DDS::OctetSeq*>& src,
                   DDS::OctetSeq& dst)
    {
      if (!private_key) return 1;

      std::vector<const DDS::OctetSeq*>::const_iterator i, n;
      size_t len = 0u;

      md_ctx = EVP_MD_CTX_new();
      if (!md_ctx) {
        OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
        return 1;
      }

      pkey_ctx = EVP_PKEY_CTX_new(private_key, NULL);
      if (!pkey_ctx) {
        OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
        return 1;
      }

      EVP_MD_CTX_init(md_ctx);

      if (1 != EVP_DigestSignInit(md_ctx, &pkey_ctx, EVP_sha256(), NULL,
                                  private_key)) {
        OPENDDS_SSL_LOG_ERR("EVP_DigestSignInit failed");
        return 1;
      }

      // Determine which signature type is being signed
      int pk_id = EVP_PKEY_id(private_key);

      if (pk_id == EVP_PKEY_RSA) {
        if (1 !=
            EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING)) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_rsa_padding failed");
          return 1;
        }

        if (1 != EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, EVP_sha256())) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_rsa_mgf1_md failed");
          return 1;
        }
      }

      n = src.end();
      for (i = src.begin(); i != n; ++i) {
        if ((*i)->length() > 0) {
          if (1 != EVP_DigestSignUpdate(md_ctx, (*i)->get_buffer(),
                                        (*i)->length())) {
            OPENDDS_SSL_LOG_ERR("EVP_DigestSignUpdate failed");
            return 1;
          }
        }
      }

      /* First call with NULL to extract size */
      if (1 != EVP_DigestSignFinal(md_ctx, NULL, &len)) {
        OPENDDS_SSL_LOG_ERR("EVP_DigestSignFinal failed");
        return 1;
      }

      /* Second call to extract the data */
      dst.length(static_cast<unsigned int>(len));
      if (1 != EVP_DigestSignFinal(md_ctx, dst.get_buffer(), &len)) {
        OPENDDS_SSL_LOG_ERR("EVP_DigestSignFinal failed");
        return 1;
      }

      // The last call to EVP_DigestSignFinal can change the value of len so
      // reassign the value to len to dst.length.  This happens when using EC
      dst.length(static_cast<unsigned int>(len));

      return 0;
    }

   private:
    EVP_PKEY* private_key;
    EVP_MD_CTX* md_ctx;
    EVP_PKEY_CTX* pkey_ctx;
  };

  int PrivateKey::sign(const std::vector<const DDS::OctetSeq*>& src,
                       DDS::OctetSeq& dst) const
  {
    sign_implementation sign(k_);
    return sign(src, dst);
  }

  EVP_PKEY* PrivateKey::EVP_PKEY_from_pem(const std::string& path,
                                          const std::string& password)
  {
    EVP_PKEY* result = NULL;

    BIO* filebuf = BIO_new_file(path.c_str(), "r");
    if (filebuf) {
      if (password != "") {
        result = PEM_read_bio_PrivateKey(filebuf, NULL, NULL,
                                         (void*)password.c_str());
        if (!result) {
          OPENDDS_SSL_LOG_ERR("PEM_read_bio_PrivateKey failed");
        }

      } else {
        result = PEM_read_bio_PrivateKey(filebuf, NULL, NULL, NULL);
        if (!result) {
          OPENDDS_SSL_LOG_ERR("PEM_read_bio_PrivateKey failed");
        }
      }

      BIO_free(filebuf);

    } else {
      std::stringstream errmsg;
      errmsg << "failed to read file '" << path << "' using BIO_new_file";
      OPENDDS_SSL_LOG_ERR(errmsg.str().c_str());
    }

    return result;
  }

  EVP_PKEY* PrivateKey::EVP_PKEY_from_pem_data(const std::string& data,
                                               const std::string& password)
  {
    DDS::OctetSeq original_bytes;

    // The minus 1 is because path contains a comma in element 0 and that
    // comma is not included in the cert string
    original_bytes.length(data.size() - 1);
    std::memcpy(original_bytes.get_buffer(), &data[1],
                original_bytes.length());

    // To appease the other DDS security implementations which
    // append a null byte at the end of the cert.
    original_bytes.length(original_bytes.length() + 1);
    original_bytes[original_bytes.length() - 1] = 0;

    EVP_PKEY* result = NULL;
    BIO* filebuf = BIO_new(BIO_s_mem());

    if (filebuf) {
      if (0 >= BIO_write(filebuf, original_bytes.get_buffer(),
                         original_bytes.length())) {
        OPENDDS_SSL_LOG_ERR("BIO_write failed");
      }

      if (password != "") {
        result = PEM_read_bio_PrivateKey(filebuf, NULL, NULL,
                                         (void*)password.c_str());

        if (!result) {
          OPENDDS_SSL_LOG_ERR("PEM_read_bio_PrivateKey failed");
        }
      } else {
        result = PEM_read_bio_PrivateKey(filebuf, NULL, NULL, NULL);

        if (!result) {
          OPENDDS_SSL_LOG_ERR("PEM_read_bio_PrivateKey failed");
        }
      }

      BIO_free(filebuf);
    } else {
      std::stringstream errmsg;
      errmsg << "failed to create data '" << data << "' using BIO_new";
      OPENDDS_SSL_LOG_ERR(errmsg.str().c_str());
    }

    return result;
  }

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

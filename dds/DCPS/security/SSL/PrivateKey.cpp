/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "PrivateKey.h"
#include "Utils.h"
#include "Err.h"
#include <openssl/pem.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      PrivateKey::PrivateKey(const std::string& uri, const std::string password) : k_(NULL)
      {
        load(uri, password);
      }

      PrivateKey::PrivateKey() : k_(NULL)
      {

      }

      PrivateKey::~PrivateKey()
      {
        if (k_) EVP_PKEY_free(k_);
      }

      PrivateKey& PrivateKey::operator=(const PrivateKey& rhs)
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

      void PrivateKey::load(const std::string& uri, const std::string& password)
      {
        if (k_) return;

        std::string path;
        URI_SCHEME s = extract_uri_info(uri, path);

        switch(s) {
          case URI_FILE:
            k_ = EVP_PKEY_from_pem(path, password);
            break;

          case URI_DATA:
          case URI_PKCS11:
          case URI_UNKNOWN:
          default:
            fprintf(stderr, "PrivateKey::load: Unsupported URI scheme in cert path '%s'\n", uri.c_str());
            break;
        }
      }

      int PrivateKey::sign(const std::vector<const DDS::OctetSeq*> & src, DDS::OctetSeq& dst)
      {
        EVP_MD_CTX* signature_ctx = EVP_MD_CTX_new();
        if (! signature_ctx) {
          OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
          return 1;
        }

        if (1 != EVP_DigestSignInit(signature_ctx, NULL, EVP_sha256(), NULL, k_)) {
          OPENDDS_SSL_LOG_ERR("EVP_DigestSignInit failed");
          return 1;
        }

        std::vector<const DDS::OctetSeq*>::const_iterator i, n = src.end();
        for (i = src.begin(); i != n; ++i) {
          EVP_DigestSignUpdate(signature_ctx, (*i)->get_buffer(), (*i)->length());
        }

        unsigned long int len = 0u;

        /* First call with NULL to extract size */
        if (1 != EVP_DigestSignFinal(signature_ctx, NULL, &len)) {
          OPENDDS_SSL_LOG_ERR("EVP_DigestSignFinal failed");
          return 1;
        }

        /* Second call to extract the data */
        dst.length(len);
        if (1 != EVP_DigestSignFinal(signature_ctx, dst.get_buffer(), &len)) {
          OPENDDS_SSL_LOG_ERR("EVP_DigestSignFinal failed");
          return 1;
        }

        EVP_MD_CTX_free(signature_ctx);

        return 0;
      }

      EVP_PKEY* PrivateKey::EVP_PKEY_from_pem(const std::string& path, const std::string& password)
      {
        EVP_PKEY* result = NULL;

        BIO* filebuf = BIO_new_file(path.c_str(), "r");
        if (filebuf) {
          if (password != "") {
              result = PEM_read_bio_PrivateKey(filebuf, NULL, NULL, (void*)password.c_str());
              if (! result) {
                OPENDDS_SSL_LOG_ERR("PEM_read_bio_PrivateKey failed");
              }

          } else {
              result = PEM_read_bio_PrivateKey(filebuf, NULL, NULL, NULL);
              if (! result) {
                OPENDDS_SSL_LOG_ERR("PEM_read_bio_PrivateKey failed");
              }
          }

          BIO_free(filebuf);

        } else {
          std::stringstream errmsg; errmsg << "failed to read file '" << path << "' using BIO_new_file";
          OPENDDS_SSL_LOG_ERR(errmsg.str());
        }

        return result;
      }

    }
  }
}

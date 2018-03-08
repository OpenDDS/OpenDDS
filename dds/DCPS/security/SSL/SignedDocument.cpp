/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "SignedDocument.h"
#include "Utils.h"
#include "Err.h"
#include <openssl/pem.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      SignedDocument::SignedDocument(const std::string& uri) : doc_(NULL)
      {
        load(uri);
      }

      SignedDocument::SignedDocument() : doc_(NULL)
      {

      }

      SignedDocument::~SignedDocument()
      {
        if (doc_) PKCS7_free(doc_);
      }

      SignedDocument& SignedDocument::operator=(const SignedDocument& rhs)
      {
        if (this != &rhs) {
            if (rhs.doc_) {
                doc_ = PKCS7_dup(rhs.doc_);

            } else {
                doc_ = NULL;
            }
        }
        return *this;
      }

      void SignedDocument::load(const std::string& uri)
      {
        if (doc_) return;

        std::string path;
        URI_SCHEME s = extract_uri_info(uri, path);

        switch(s) {
          case URI_FILE:
            doc_ = PKCS7_from_SMIME(path);
            break;

          case URI_DATA:
          case URI_PKCS11:
          case URI_UNKNOWN:
          default:
            fprintf(stderr, "SignedDocument::load: Unsupported URI scheme '%s'\n", uri.c_str());
            break;
        }
      }

      int SignedDocument::get_content(std::string& dst)
      {
        if (!plaintext_.empty()) {
          dst = plaintext_;
          return 0;
        }

        int result = 1;

        dst.clear();

        if (doc_) {
          if (content_) {
            unsigned char tmp[32] = {0};
            int len = 0;
            while ((len = BIO_read(content_, tmp, sizeof(tmp))) > 0) {
              dst.insert(dst.end(), tmp, tmp + len);
              result = 0;
            }

          } else {
            OPENDDS_SSL_LOG_ERR("PKCS7_decrypt failed");
          }

          BIO_free(content_);
          content_ = 0;
          plaintext_ = dst;
        }

        return result;
      }

      int SignedDocument::verify_signature(const Certificate& cert)
      {
        int result = 1; /* 1 = failure, 0 = success */

        /*
         * TODO something similar to the implementation of Certificate.validate(...)
         * using:
         *
         * int PKCS7_verify(PKCS7 *p7, STACK_OF(X509) *certs, X509_STORE *store, BIO *indata, BIO *out, int flags);
         */

        return result;
      }

      PKCS7* SignedDocument::PKCS7_from_SMIME(const std::string& path)
      {
        PKCS7* result = NULL;

        BIO* filebuf = BIO_new_file(path.c_str(), "r");
        if (filebuf) {

          result = SMIME_read_PKCS7(filebuf, &content_);
          if (! result) {
            OPENDDS_SSL_LOG_ERR("SMIME_read_PKCS7 failed");
            content_ = 0;
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

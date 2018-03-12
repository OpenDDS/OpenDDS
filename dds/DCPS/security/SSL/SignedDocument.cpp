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

      SignedDocument::SignedDocument(const std::string& uri) : doc_(NULL), content_(NULL), plaintext_("")
      {
        load(uri);
      }

      SignedDocument::SignedDocument() : doc_(NULL), content_(NULL), plaintext_("")
      {

      }

      SignedDocument::SignedDocument(const DDS::OctetSeq& src) : doc_(NULL), content_(NULL), plaintext_("")
      {
        deserialize(src);
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
            if (doc_) cache_plaintext();
            break;

          case URI_DATA:
          case URI_PKCS11:
          case URI_UNKNOWN:
          default:
            fprintf(stderr, "SignedDocument::load: Unsupported URI scheme '%s'\n", uri.c_str());
            break;
        }
      }

      void SignedDocument::get_content(std::string& dst)
      {
        dst = plaintext_;
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

      int SignedDocument::serialize(std::vector<unsigned char>& dst)
      {
        int result = 1;

        if (doc_) {

          BIO* buffer = BIO_new(BIO_s_mem());
          if (buffer) {

            if (1 == SMIME_write_PKCS7(buffer, doc_, NULL, 0)) {

              unsigned char tmp[32] = {0};
              int len = 0;
              while ((len = BIO_read(buffer, tmp, sizeof(tmp))) > 0) {
                dst.insert(dst.end(), tmp, tmp + len);
                result = 0;
              }

            } else {
                OPENDDS_SSL_LOG_ERR("failed to write X509 to PEM");
            }

            BIO_free(buffer);

          } else {
            OPENDDS_SSL_LOG_ERR("failed to allocate buffer with BIO_new");
          }
        }

        return result;
      }

      int SignedDocument::serialize(DDS::OctetSeq& dst)
      {
        std::vector<unsigned char> tmp;
        int err = serialize(tmp);
        if (! err) {
            dst.length(tmp.size());
            for (size_t i = 0; i < tmp.size(); ++i) {
              dst[i] = tmp[i];
            }
        }
        return err;
      }

      int SignedDocument::deserialize(const DDS::OctetSeq& src)
      {
        int result = 1;

        if (! doc_) {
          if (src.length() > 0) {

            BIO* buffer = BIO_new(BIO_s_mem());
            if (buffer) {

              int len = BIO_write(buffer, src.get_buffer(), src.length());
              if (len > 0) {

                doc_ = SMIME_read_PKCS7(buffer, &content_);
                if (doc_) {
                    result = 0;
                    cache_plaintext();

                } else {
                    OPENDDS_SSL_LOG_ERR("failed to read SMIME from BIO");
                    content_ = NULL;
                }

              } else {
                  OPENDDS_SSL_LOG_ERR("failed to write OctetSeq to BIO");
              }

              BIO_free(buffer);

            } else {
              OPENDDS_SSL_LOG_ERR("failed to allocate buffer with BIO_new");
            }

          } else {
            fprintf(stderr, "Certificate::deserialize: Error, source OctetSeq contains no data\n");
          }

        } else {
          fprintf(stderr, "Certificate::deserialize: Error, an X509 certificate has already been loaded\n");
        }

        return result;
      }

      int SignedDocument::cache_plaintext()
      {
        int result = 1;

        if (doc_) {
          if (content_) {
            unsigned char tmp[32] = {0};
            int len = 0;
            while ((len = BIO_read(content_, tmp, sizeof(tmp))) > 0) {
              plaintext_.insert(plaintext_.end(), tmp, tmp + len);
              result = 0;
            }

          } else {
            OPENDDS_SSL_LOG_ERR("PKCS7_decrypt failed");
          }

          BIO_free(content_);
          content_ = 0;
        }

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
            content_ = NULL;

          }

          BIO_free(filebuf);

        } else {
          std::stringstream errmsg; errmsg << "failed to read file '" << path << "' using BIO_new_file";
          OPENDDS_SSL_LOG_ERR(errmsg.str());
        }

        return result;
      }

      bool operator==(const SignedDocument& lhs, const SignedDocument& rhs)
      {
        return lhs.plaintext_ == rhs.plaintext_;
      }

    }
  }
}

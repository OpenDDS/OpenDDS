/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "Certificate.h"
#include "Utils.h"
#include <cstring>
#include <cerrno>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      Certificate::Certificate(const std::string& uri, const std::string& password) : x_(NULL)
      {
        load(uri, password);
      }

      Certificate::Certificate() : x_(NULL)
      {

      }

      Certificate::~Certificate()
      {
        if (x_) X509_free(x_);
      }

      Certificate& Certificate::operator=(const Certificate& rhs)
      {
        if (this != &rhs) {
            if (rhs.x_) {
                x_ = rhs.x_;
                X509_up_ref(x_);

            } else {
                x_ = NULL;
            }
        }
        return *this;
      }

      void Certificate::load(const std::string& uri, const std::string& password)
      {
        if (x_) return;

        std::string path;
        URI_SCHEME s = extract_uri_info(uri, path);

        switch(s) {
          case URI_FILE:
            x_ = x509_from_pem(path, password);
            break;

          case URI_DATA:
          case URI_PKCS11:
          case URI_UNKNOWN:
          default:
            /* TODO use ACE logging */
            fprintf(stderr, "Certificate::Certificate: Error, unsupported URI scheme in cert path '%s'\n", uri.c_str());
            break;
        }
      }

      int Certificate::validate(Certificate& ca, unsigned long int flags)
      {
        int result = X509_V_ERR_UNSPECIFIED;

        if (x_) {
            if (ca.x_) {

                X509_STORE* certs = X509_STORE_new();
                if (certs) {
                    X509_STORE_add_cert(certs, ca.x_);

                    X509_STORE_CTX* validation_ctx = X509_STORE_CTX_new();
                    if (validation_ctx) {
                        X509_STORE_CTX_init(validation_ctx, certs, x_, NULL);
                        X509_STORE_CTX_set_flags(validation_ctx, flags);

                        if (X509_verify_cert(validation_ctx) == 1) {
                            result = X509_V_OK;

                        } else {
                            int err = X509_STORE_CTX_get_error(validation_ctx),
                                depth = X509_STORE_CTX_get_error_depth(validation_ctx);

                            fprintf(stderr,
                                    "Certificate::verify: Error '%s' occurred using cert at depth '%d', validation failed.",
                                    X509_verify_cert_error_string(err),
                                    depth);

                            result = err;
                        }

                        X509_STORE_CTX_free(validation_ctx);
                    }

                    X509_STORE_free(certs);

                } else {
                    fprintf(stderr, "Certificate::verify: Error, failed to create X509_STORE");
                }

            } else {
                fprintf(stderr, "Certificate::verify: Error, passed-in CA has not loaded a certificate");
            }

        } else {
            fprintf(stderr, "Certificate::verify: Error, a certificate must be loaded before it can be verified");
        }

        return result;
      }

      X509* Certificate::x509_from_pem(const std::string& path, const std::string& password)
      {
        X509* result = NULL;

        FILE* fp = fopen(path.c_str(), "r");
        if (fp) {
          if (password != "") {
              result = PEM_read_X509_AUX(fp, NULL, NULL, (void*)password.c_str());

          } else {
              result = PEM_read_X509_AUX(fp, NULL, NULL, NULL);
          }

          fclose(fp);

        } else {
          /* TODO use ACE logging */
          fprintf(stderr, "Certificate::x509_from_pem: Error '%s' reading file '%s'\n", strerror(errno), path.c_str());
        }

        return result;
      }

      std::ostream& operator<<(std::ostream& lhs, Certificate& rhs)
      {
        X509* x = rhs.get();

        if (x) {
            lhs << "Certificate: { is_ca? '" << (X509_check_ca(x) ? "yes": "no") << "'; }";

        } else {
            lhs << "NULL";
        }
        return lhs;
      }
    }
  }
}


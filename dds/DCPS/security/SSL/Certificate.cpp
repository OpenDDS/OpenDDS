/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "Certificate.h"
#include "Utils.h"
#include "Err.h"
#include <algorithm>
#include <cstring>
#include <openssl/pem.h>
#include <openssl/x509v3.h>


namespace OpenDDS {
  namespace Security {
    namespace SSL {

      Certificate::Certificate(const std::string& uri, const std::string& password) : x_(NULL)
      {
        load(uri, password);
      }

      Certificate::Certificate(const DDS::OctetSeq& src) : x_(NULL)
      {
        deserialize(src);
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
            fprintf(stderr, "Certificate::Certificate: Error, unsupported URI scheme in cert path '%s'\n", uri.c_str());
            break;
        }
      }

      int Certificate::validate(Certificate& ca, unsigned long int flags) const
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
                                "Certificate::verify: Error '%s' occurred using cert at depth '%d', validation failed.\n",
                                X509_verify_cert_error_string(err),
                                depth);

                        result = err;
                    }

                    X509_STORE_CTX_free(validation_ctx);
                }

                X509_STORE_free(certs);

            } else {
                OPENDDS_SSL_LOG_ERR("failed to create X509_STORE");
            }

          } else {
              fprintf(stderr, "Certificate::verify: Error, passed-in CA has not loaded a certificate\n");
          }

        } else {
            fprintf(stderr, "Certificate::verify: Error, a certificate must be loaded before it can be verified\n");
        }

        return result;
      }

      int Certificate::verify_signature(const DDS::OctetSeq& src, const std::vector<const DDS::OctetSeq*>& expected_contents)
      {
        int err = 0;
        std::vector<const DDS::OctetSeq*>::const_iterator i, n;
        EVP_PKEY* pubkey = NULL;
        EVP_MD_CTX* verify_ctx = NULL;
        const EVP_MD* mdtype = NULL;

        pubkey = X509_get_pubkey(x_);
        if (! pubkey) {
          OPENDDS_SSL_LOG_ERR("X509_get_pubkey failed");
          goto error;
        }

        verify_ctx = EVP_MD_CTX_new();
        if (! verify_ctx) {
          OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
          goto error;
        }

        mdtype = EVP_get_digestbynid(NID_sha256WithRSAEncryption);
        if (1 != EVP_DigestVerifyInit(verify_ctx, NULL, mdtype, NULL, pubkey)) {
          OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyInit failed");
          goto error;
        }

        n = expected_contents.end();
        for (i = expected_contents.begin(); i != n; ++i) {
          if ((*i)->length() > 0) {
            if (1 != EVP_DigestVerifyUpdate(verify_ctx, (*i)->get_buffer(), (*i)->length())) {
              OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyUpdate failed");
              goto error;
            }
          }
        }

        err = EVP_DigestVerifyFinal(verify_ctx, src.get_buffer(), src.length());
        if (0 == err) {
          goto error; /* Verification failed, but no error occurred*/

        } else if (1 != err) {
          OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyFinal failed");
          goto error;
        }

        return 0;

        error:
          if (pubkey) EVP_PKEY_free(pubkey);
          return 1;
      }

      int Certificate::subject_name_to_str(std::string& dst, unsigned long flags) const
      {
        int result = 1, len = 0;
        char* tmp = NULL;

        dst.clear();

        if (x_) {

          /* Do not free name! */
          X509_NAME* name = X509_get_subject_name(x_);
          if (name) {

            BIO* buffer = BIO_new(BIO_s_mem());
            if (buffer) {

              len = X509_NAME_print_ex(buffer, name, 0, flags);
              if (len > 0) {

                tmp = new char[len + 1]; /* Add 1 for null-terminator */
                len = BIO_gets(buffer, tmp, len + 1 /* Writes up to len-1 and adds null-terminator */);
                if (len > 0) {
                  dst = tmp;
                  result = 0;

                } else {
                    OPENDDS_SSL_LOG_ERR("failed to write BIO to string");
                }

              } else {
                  OPENDDS_SSL_LOG_ERR("failed to read X509_NAME into BIO buffer");
              }

              BIO_free(buffer);
            }
          }
        }

        if (tmp) delete[] tmp;

        return result;
      }

      int Certificate::subject_name_digest(std::vector<unsigned char>& dst) const
      {
        int result = 1;
        unsigned int len = 0;
        unsigned char* buffer = NULL;

        dst.clear();

        if (x_) {

          /* Do not free name! */
          X509_NAME* name = X509_get_subject_name(x_);
          if (name) {

            buffer = new unsigned char[EVP_MAX_MD_SIZE];
            if (X509_NAME_digest(name, EVP_sha256(), buffer, &len) == 1) {
              dst.insert(dst.begin(), buffer, buffer + len);
              result = 0;
            }
          }
        }

        if (buffer) delete[] buffer;

        return result;
      }

      int Certificate::algorithm(std::string& dst) const
      {
        int result = 1, keylen = 0;

        dst.clear();

        if (x_) {

          EVP_PKEY* pkey = X509_get_pubkey(x_);
          if (pkey) {

            RSA* rsa = EVP_PKEY_get1_RSA(pkey);
            if (rsa) {

              keylen = RSA_bits(rsa);
              if (keylen == 2048) {

                dst = "RSA-2048";
                result = 0;

              } else {
                fprintf(stderr,
                        "Certificate::algorithm: Error, currently RSA-2048 is the only supported algorithm; "
                        "received RSA cert with '%d' bits\n",
                        keylen);
              }

              RSA_free(rsa);

            } else {

              /* TODO add support for "EC-prime256v1" */

              fprintf(stderr, "Certificate::algorithm: Error, only RSA-2048 is currently supported\n");
            }

          } else {
              fprintf(stderr, "Certificate::algorithm: Error, failed to get pubkey from X509 cert\n");
          }

          EVP_PKEY_free(pkey);
        }

        return result;
      }

      X509* Certificate::x509_from_pem(const std::string& path, const std::string& password)
      {
        X509* result = NULL;

        BIO* filebuf = BIO_new_file(path.c_str(), "r");
        if (filebuf) {
          if (password != "") {
            result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL, (void*)password.c_str());
            if (! result) {
                OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
            }

          } else {
            result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL, NULL);
            if (! result) {
                OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
            }
          }

          BIO_free(filebuf);

        } else {
          std::stringstream errmsg; errmsg << "failed to read file '" << path << "' using BIO_new_file";
          OPENDDS_SSL_LOG_ERR(errmsg.str());
        }

        return result;
      }

      int Certificate::serialize(std::vector<unsigned char>& dst) const
      {
        int result = 1;

        if (x_) {

          BIO* buffer = BIO_new(BIO_s_mem());
          if (buffer) {

            if (1 == PEM_write_bio_X509(buffer, x_)) {

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

      int Certificate::serialize(DDS::OctetSeq& dst) const
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

      int Certificate::deserialize(const DDS::OctetSeq& src)
      {
        int result = 1;

        if (! x_) {
          if (src.length() > 0) {

            BIO* buffer = BIO_new(BIO_s_mem());
            if (buffer) {

              int len = BIO_write(buffer, src.get_buffer(), src.length());
              if (len > 0) {
                x_ = PEM_read_bio_X509_AUX(buffer, NULL, NULL, NULL);

                if (x_) {
                    result = 0;

                } else {
                    OPENDDS_SSL_LOG_ERR("failed to read X509 from BIO");
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

      std::ostream& operator<<(std::ostream& lhs, const Certificate& rhs)
      {
        if (rhs.x_) {
            lhs << "Certificate: { is_ca? '" << (X509_check_ca(rhs.x_) ? "yes": "no") << "'; }";

        } else {
            lhs << "NULL";
        }
        return lhs;
      }

      bool operator==(const Certificate& lhs, const Certificate& rhs)
      {
        return 0 == X509_cmp(lhs.x_, rhs.x_);
      }
    }
  }
}

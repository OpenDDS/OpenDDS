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
#include "../OpenSSL_legacy.h" // Must come after all other OpenSSL includes


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
#ifndef OPENSSL_LEGACY
                X509_up_ref(x_);
#endif

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
            load_cert_bytes(path);
            x_ = x509_from_pem(original_bytes_, password);
            break;

          case URI_DATA:
			  load_cert_data_bytes(path);
			  x_ = x509_from_pem(original_bytes_, password);
			  break;

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

      class verify_implementation
      {
      public:
          verify_implementation(EVP_PKEY* pkey) :
             public_key(pkey), md_ctx(NULL), pkey_ctx(NULL)
        {

        }
        ~verify_implementation()
        {
          EVP_MD_CTX_free(md_ctx);
        }

        int operator()(const DDS::OctetSeq& src, const std::vector<const DDS::OctetSeq*>& expected_contents)
        {
          if (! public_key) return 1;

          int pk_id = 0;
          std::vector<const DDS::OctetSeq*>::const_iterator i, n;

          md_ctx = EVP_MD_CTX_new();
          if (! md_ctx) {
            OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
            return 1;
          }

          pkey_ctx = EVP_PKEY_CTX_new(public_key, NULL);
          if (! pkey_ctx) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
            return 1;
          }

          EVP_MD_CTX_init(md_ctx);

          if (1 != EVP_DigestVerifyInit(md_ctx, &pkey_ctx, EVP_sha256(), NULL, public_key)) {
            OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyInit failed");
            return 1;
          }

          // Determine which signature type is being verified
          pk_id = EVP_PKEY_id(public_key);

          if (pk_id == EVP_PKEY_RSA)
          {
              if (1 != EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING)) 
              {
                  OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_rsa_padding failed");
                  return 1;
              }

              if (1 != EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, EVP_sha256())) 
              {
                  OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_rsa_mgf1_md failed");
                  return 1;
              }
          }

          n = expected_contents.end();
          for (i = expected_contents.begin(); i != n; ++i) {
            if ((*i)->length() > 0) {
              if (1 != EVP_DigestVerifyUpdate(md_ctx, (*i)->get_buffer(), (*i)->length())) {
                OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyUpdate failed");
                return 1;
              }
            }
          }

          int err = EVP_DigestVerifyFinal(md_ctx, src.get_buffer(), src.length());
          if (0 == err) {
            return 1; // Verification failed, but no error occurred

          } else if (1 != err) {
            OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyFinal failed");
            return 1;
          }

          return 0;
        }

      private:
        EVP_PKEY* public_key;
        EVP_MD_CTX* md_ctx;
        EVP_PKEY_CTX* pkey_ctx;
      };

      int Certificate::verify_signature(const DDS::OctetSeq& src, const std::vector<const DDS::OctetSeq*>& expected_contents) const
      {
          verify_implementation verify(X509_get_pubkey(x_));
          return verify(src, expected_contents);
      }

      int Certificate::subject_name_to_str(std::string& dst, unsigned long flags) const
      {
        int result = 1, len = 0;

        dst.clear();

        if (x_) {

          /* Do not free name! */
          X509_NAME* name = X509_get_subject_name(x_);
          if (name) {

            BIO* buffer = BIO_new(BIO_s_mem());
            if (buffer) {

              len = X509_NAME_print_ex(buffer, name, 0, flags);
              if (len > 0) {

                std::vector<char> tmp(len + 1); // BIO_gets will add null hence +1
                len = BIO_gets(buffer, &tmp[0], len + 1);
                if (len > 0) {
                  std::copy(tmp.begin(),
                            tmp.end() - 1, // But... string inserts a null so it's not needed
                            std::back_inserter(dst));
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

        return result;
      }

      int Certificate::subject_name_digest(std::vector<CORBA::Octet>& dst) const
      {
        dst.clear();

        if (! x_) return 1;

        /* Do not free name! */
        X509_NAME* name = X509_get_subject_name(x_);
        if (NULL == name) {
          OPENDDS_SSL_LOG_ERR("X509_get_subject_name failed");
          return 1;
        }

        std::vector<CORBA::Octet> tmp(EVP_MAX_MD_SIZE);

        unsigned int len = 0;
        if (1 != X509_NAME_digest(name, EVP_sha256(), &tmp[0], &len)) {
          OPENDDS_SSL_LOG_ERR("X509_NAME_digest failed");
          return 1;
        }

        dst.insert(dst.begin(), tmp.begin(), tmp.begin() + len);

        return 0;
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

                EC_KEY* eckey = EVP_PKEY_get1_EC_KEY(pkey);

                if (eckey)
                {
                    dst = "EC-prime256v1";
                    result = 0;
                }
                else
                {
                    fprintf(stderr, "Certificate::algorithm: Error, only RSA-2048 or EC-prime256v1 are currently supported\n");
                }

                EC_KEY_free(eckey);
            }

          } else {
              fprintf(stderr, "Certificate::algorithm: Error, failed to get pubkey from X509 cert\n");
          }

          EVP_PKEY_free(pkey);
        }

        return result;
      }

      void Certificate::load_cert_bytes(const std::string& path)
      {
        std::vector<CORBA::Octet> chunks;
        CORBA::Octet chunk[32] = {0};

        FILE* fp = fopen(path.c_str(), "r");
        if (fp) {
          size_t count = 0u;
          while((count = fread(chunk, sizeof(chunk[0]), sizeof(chunk), fp))) {
            chunks.insert(chunks.end(),
                          chunk,
                          chunk + count);
          }

          if (ferror(fp)) {
              fprintf(stderr,
                      "LocalAuthCredentialData::load_permissions_file: Error '%s' occured while reading file '%s'\n",
                      std::strerror(errno),
                      path.c_str());
          } else {
              original_bytes_.length(chunks.size());
              std::memcpy(original_bytes_.get_buffer(), &chunks[0], original_bytes_.length());
          }

          // To appease the other DDS security implementations which
          // append a null byte at the end of the cert.
          original_bytes_.length(original_bytes_.length() + 1);
          original_bytes_[original_bytes_.length() - 1] = 0;

          fclose(fp);
        }
      }

	  void Certificate::load_cert_data_bytes(const std::string& data)
	  {
		  // The minus 1 is because path contains a comma in element 0 and that comma
		  // is not included in the cert string
		  original_bytes_.length(data.size() - 1);
		  std::memcpy(original_bytes_.get_buffer(), &data[1], original_bytes_.length());

		  // To appease the other DDS security implementations which
		  // append a null byte at the end of the cert.
		  original_bytes_.length(original_bytes_.length() + 1);
		  original_bytes_[original_bytes_.length() - 1] = 0;
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

      X509* Certificate::x509_from_pem(const DDS::OctetSeq& bytes, const std::string& password)
      {
        X509* result = NULL;

        BIO* filebuf = BIO_new(BIO_s_mem());
        do {
          if (filebuf) {
            if (0 >= BIO_write(filebuf, bytes.get_buffer(), bytes.length())) {
                OPENDDS_SSL_LOG_ERR("BIO_write failed");
                break;
            }
            if (password != "") {
              result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL, (void*)password.c_str());
              if (! result) {
                  OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
                  break;
              }

            } else {
              result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL, NULL);
              if (! result) {
                  OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
                  break;
              }
            }

          } else {
            std::stringstream errmsg; errmsg << "BIO_new failed";
            OPENDDS_SSL_LOG_ERR(errmsg.str());
            break;
          }

        } while(0);

        BIO_free(filebuf);

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
                    original_bytes_ = src;
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

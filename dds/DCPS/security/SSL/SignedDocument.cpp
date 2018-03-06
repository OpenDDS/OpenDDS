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

      SignedDocument::SignedDocument(const std::string& uri, const std::string password) : doc_(NULL)
      {
        load(uri, password);
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

      void SignedDocument::load(const std::string& uri, const std::string& password)
      {
        if (doc_) return;

        std::string path;
        URI_SCHEME s = extract_uri_info(uri, path);

        switch(s) {
          case URI_FILE:
            doc_ = PKCS7_from_pem(path, password);
            break;

          case URI_DATA:
          case URI_PKCS11:
          case URI_UNKNOWN:
          default:
            fprintf(stderr, "SignedDocument::load: Unsupported URI scheme '%s'\n", uri.c_str());
            break;
        }
      }

      PKCS7* SignedDocument::PKCS7_from_pem(const std::string& path, const std::string& password)
      {
        PKCS7* result = NULL;

        BIO* filebuf = BIO_new_file(path.c_str(), "r");
        if (filebuf) {
          if (password != "") {
              result = PEM_read_bio_PKCS7(filebuf, NULL, NULL, (void*)password.c_str());

          } else {
              result = PEM_read_bio_PKCS7(filebuf, NULL, NULL, NULL);
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

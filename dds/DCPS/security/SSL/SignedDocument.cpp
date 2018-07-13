/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "SignedDocument.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "Err.h"
#include <openssl/pem.h>
#include <cstring>
#include <sstream>

namespace OpenDDS {
namespace Security {
namespace SSL {

  SignedDocument::SignedDocument(const std::string& uri)
    : doc_(NULL), content_(NULL), plaintext_("")
  {
    load(uri);
  }

  SignedDocument::SignedDocument()
    : doc_(NULL), content_(NULL), plaintext_("")
  {
  }

  SignedDocument::SignedDocument(const DDS::OctetSeq& src)
    : doc_(NULL), content_(NULL), plaintext_("")
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
    using namespace CommonUtilities;

    if (doc_) return;

    URI uri_info(uri);

    switch (uri_info.scheme) {
      case URI::URI_FILE:
        doc_ = PKCS7_from_SMIME_file(uri_info.everything_else);
        if (doc_) cache_plaintext();
        break;

      case URI::URI_DATA:
        deserialize(uri_info.everything_else);
        break;

      case URI::URI_PKCS11:
      case URI::URI_UNKNOWN:
      default:
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) SSL::SignedDocument::load: WARNING: Unsupported URI scheme '%C'\n"),
                   uri.c_str()));
        break;
    }
  }

  void SignedDocument::get_content(std::string& dst) const { dst = plaintext_; }

  class verify_signature_impl
  {
   public:
    verify_signature_impl(PKCS7* doc, const std::string& content)
      : doc_(doc),
        content_(content),
        store_(NULL),
        store_ctx_(NULL),
        reader_(NULL)
    {
      if (NULL == (store_ = X509_STORE_new())) {
        OPENDDS_SSL_LOG_ERR("X509_STORE_new failed");
      }
      if (NULL == (store_ctx_ = X509_STORE_CTX_new())) {
        OPENDDS_SSL_LOG_ERR("X509_STORE_CTX_new failed");
      }
      if (NULL == (reader_ = BIO_new(BIO_s_mem()))) {
        OPENDDS_SSL_LOG_ERR("BIO_new failed");
      }
    }

    ~verify_signature_impl()
    {
      X509_STORE_CTX_free(store_ctx_);
      X509_STORE_free(store_);
      BIO_free(reader_);
    }

    int operator()(const Certificate& ca, unsigned long int flags = 0)
    {
      if (!doc_) return 1;
      if (0 == content_.length()) return 1;

      if (1 != X509_STORE_add_cert(store_, ca.x_)) {
        OPENDDS_SSL_LOG_ERR("X509_STORE_add_cert failed");
        return 1;
      }

      size_t len = BIO_write(reader_, content_.c_str(), content_.length());
      if (len <= 0) {
        OPENDDS_SSL_LOG_ERR("BIO_write failed");
        return 1;
      }

      if (1 != PKCS7_verify(doc_, NULL, store_, reader_, NULL, flags)) {
        OPENDDS_SSL_LOG_ERR("PKCS7_verify failed");
        return 1;
      }
      return 0;
    }

   private:
    PKCS7* doc_;
    const std::string& content_;

    X509_STORE* store_;
    X509_STORE_CTX* store_ctx_;
    BIO* reader_;
  };

  int SignedDocument::verify_signature(const Certificate& ca) const
  {
    verify_signature_impl verify(doc_, plaintext_);
    return verify(ca);
  }

  int SignedDocument::serialize(std::vector<unsigned char>& dst) const
  {
    int result = 1;

    if (doc_) {
      BIO* buffer = BIO_new(BIO_s_mem());
      if (buffer) {
        if (1 == SMIME_write_PKCS7(buffer, doc_, NULL, 0)) {
          unsigned char tmp[32] = { 0 };
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

  int SignedDocument::serialize(DDS::OctetSeq& dst) const
  {
    std::vector<unsigned char> tmp;
    int err = serialize(tmp);
    if (!err) {
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

    if (!doc_) {
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
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) SSL::Certificate::deserialize: WARNING, source OctetSeq contains no data\n")));
      }

    } else {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) SSL::Certificate::deserialize: WARNING, an X509 certificate has already been loaded\n")));
    }

    return result;
  }

  int SignedDocument::deserialize(const std::string& src)
  {
    if (doc_) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) SSL::Certificate::deserialize: WARNING, an X509 certificate has already been loaded\n")));
      return 1;
    }

    doc_ = PKCS7_from_data(src);
    if (doc_) {
      return cache_plaintext();

    } else {
      return 1;
    }
  }

  int SignedDocument::cache_plaintext()
  {
    int result = 1;

    if (doc_) {
      if (content_) {
        unsigned char tmp[32] = { 0 };
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

  PKCS7* SignedDocument::PKCS7_from_SMIME_file(const std::string& path)
  {
    PKCS7* result = NULL;

    BIO* filebuf = BIO_new_file(path.c_str(), "rb");
    if (filebuf) {
      result = SMIME_read_PKCS7(filebuf, &content_);
      if (!result) {
        OPENDDS_SSL_LOG_ERR("SMIME_read_PKCS7 failed");
        content_ = NULL;
      }

      BIO_free(filebuf);

    } else {
      std::stringstream errmsg;
      errmsg << "failed to read file '" << path << "' using BIO_new_file";
      OPENDDS_SSL_LOG_ERR(errmsg.str().c_str());
    }

    return result;
  }

  PKCS7* SignedDocument::PKCS7_from_data(const std::string& s_mime_data)
  {
    DDS::OctetSeq original_bytes;

    // The minus 1 is because path contains a comma in element 0 and that
    // comma is not included in the cert string
    original_bytes.length(s_mime_data.size() - 1);
    std::memcpy(original_bytes.get_buffer(), &s_mime_data[1],
                original_bytes.length());

    // To appease the other DDS security implementations which
    // append a null byte at the end of the cert.
    original_bytes.length(original_bytes.length() + 1);
    original_bytes[original_bytes.length() - 1] = 0;

    PKCS7* result = NULL;
    BIO* filebuf = BIO_new(BIO_s_mem());

    if (filebuf) {
      if (0 >= BIO_write(filebuf, original_bytes.get_buffer(),
                         original_bytes.length())) {
        OPENDDS_SSL_LOG_ERR("BIO_write failed");
      }

      result = SMIME_read_PKCS7(filebuf, &content_);

      if (!result) {
        OPENDDS_SSL_LOG_ERR("SMIME_read_PKCS7 failed");
        content_ = NULL;
      }

      BIO_free(filebuf);
    } else {
      std::stringstream errmsg;
      errmsg << "failed to create data '" << s_mime_data << "' using BIO_new";
      OPENDDS_SSL_LOG_ERR(errmsg.str().c_str());
    }

    return result;
  }

  bool operator==(const SignedDocument& lhs, const SignedDocument& rhs)
  {
    return lhs.plaintext_ == rhs.plaintext_;
  }

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

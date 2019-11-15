/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "Certificate.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/SequenceIterator.h"
#include "Err.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <cerrno>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include "../OpenSSL_legacy.h"  // Must come after all other OpenSSL includes
#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

Certificate::Certificate(const std::string& uri,
                         const std::string& password)
  : x_(NULL), original_bytes_(), dsign_algo_("")
{
  DDS::Security::SecurityException ex;
  if (! load(ex, uri, password)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) %C\n", ex.message.in()));
  }
}

Certificate::Certificate(const DDS::OctetSeq& src)
  : x_(NULL), original_bytes_(), dsign_algo_("")
{
  deserialize(src);
}

Certificate::Certificate()
  : x_(NULL), original_bytes_(), dsign_algo_("")
{
}

Certificate::Certificate(const Certificate& other)
  : x_(NULL), original_bytes_(), dsign_algo_("")
{
  if (0 < other.original_bytes_.length()) {
    deserialize(other.original_bytes_);
  }
}

Certificate::~Certificate()
{
  if (x_) {
    X509_free(x_);
  }
}

Certificate& Certificate::operator=(const Certificate& rhs)
{
  if (this != &rhs) {
    if (rhs.x_ && (0 < rhs.original_bytes_.length())) {
      deserialize(rhs.original_bytes_);
    }

  }
  return *this;
}

bool Certificate::load(DDS::Security::SecurityException& ex,
                       const std::string& uri,
                       const std::string& password)
{
  using namespace CommonUtilities;

  if (x_) {
    set_security_error(ex, -1, 0, "SSL::Certificate::load: WARNING: document already loaded");
    return false;
  }

  URI uri_info(uri);

  switch (uri_info.scheme) {
  case URI::URI_FILE:
    load_cert_bytes(uri_info.everything_else);
    x_ = x509_from_pem(original_bytes_, password);
    break;

  case URI::URI_DATA:
    load_cert_data_bytes(uri_info.everything_else);
    x_ = x509_from_pem(original_bytes_, password);
    break;

  case URI::URI_PKCS11:
  case URI::URI_UNKNOWN:
  default:
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SSL::Certificate::load: WARNING: Unsupported URI scheme\n"));

    break;
  }

  if (!loaded()) {
    std::stringstream msg;
    msg << "SSL::Certificate::load: WARNING: Failed to load document supplied "
      "with URI '"  << uri << "'";
    set_security_error(ex, -1, 0, msg.str().c_str());
    return false;
  }

  const int err = cache_dsign_algo();
  if (err) {
    set_security_error(ex, -1, 0, "SSL::Certificate::load: WARNING: Failed to cache signature algorithm");
    return false;
  }
  return true;
}

int Certificate::validate(const Certificate& ca, unsigned long int flags) const
{
  if (!x_) {
    ACE_ERROR_RETURN((LM_WARNING,
                      ACE_TEXT("(%P|%t) SSL::Certificate::verify: WARNING, a ")
                      ACE_TEXT("certificate must be loaded before it can be verified\n")), 1);
  }

  if (!ca.x_) {
    ACE_ERROR_RETURN((LM_WARNING,
                      ACE_TEXT("(%P|%t) SSL::Certificate::verify: WARNING, passed-in ")
                      ACE_TEXT("CA has not loaded a certificate\n")), 1);
  }

  X509_STORE* const certs = X509_STORE_new();
  if (!certs) {
    OPENDDS_SSL_LOG_ERR("failed to create X509_STORE");
    return 1;
  }

  X509_STORE_add_cert(certs, ca.x_);

  X509_STORE_CTX* validation_ctx = X509_STORE_CTX_new();
  if (!validation_ctx) {
    X509_STORE_free(certs);
    return 1;
  }

  X509_STORE_CTX_init(validation_ctx, certs, x_, NULL);
  X509_STORE_CTX_set_flags(validation_ctx, flags);

  int result =
    X509_verify_cert(validation_ctx) == 1
    ? X509_V_OK : 1; // X509_V_ERR_UNSPECIFIED is not provided by all OpenSSL versions

  if (result == 1) {
    const int err = X509_STORE_CTX_get_error(validation_ctx),
      depth = X509_STORE_CTX_get_error_depth(validation_ctx);
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) SSL::Certificate::verify: WARNING '%C' occurred using cert at ")
               ACE_TEXT("depth '%i', validation failed.\n"),
               X509_verify_cert_error_string(err), depth));
    result = err;
  }

  X509_STORE_CTX_free(validation_ctx);
  X509_STORE_free(certs);
  return result;
}

class verify_implementation
{
public:
  explicit verify_implementation(EVP_PKEY* pkey)
    : public_key(pkey), md_ctx(NULL), pkey_ctx(NULL)
  {
  }

  ~verify_implementation() { EVP_MD_CTX_free(md_ctx); }

  int operator()(const DDS::OctetSeq& src,
                 const std::vector<const DDS::OctetSeq*>& expected_contents)
  {
    if (!public_key) return 1;

    int pk_id = 0;
    std::vector<const DDS::OctetSeq*>::const_iterator i, n;

    md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
      OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
      return 1;
    }

    EVP_MD_CTX_init(md_ctx);

    if (1 != EVP_DigestVerifyInit(md_ctx, &pkey_ctx, EVP_sha256(), NULL,
                                  public_key)) {
      OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyInit failed");
      return 1;
    }

    // Determine which signature type is being verified
    pk_id = EVP_PKEY_id(public_key);

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

    n = expected_contents.end();
    for (i = expected_contents.begin(); i != n; ++i) {
      if ((*i)->length() > 0) {
        if (1 != EVP_DigestVerifyUpdate(md_ctx, (*i)->get_buffer(),
                                        (*i)->length())) {
          OPENDDS_SSL_LOG_ERR("EVP_DigestVerifyUpdate failed");
          return 1;
        }
      }
    }

#ifdef OPENSSL_V_1_0
    // some versions of OpenSSL take a pointer to non-const
    unsigned char* buffer = const_cast<unsigned char*>(src.get_buffer());
#else
    const unsigned char* buffer = src.get_buffer();
#endif
    const int err = EVP_DigestVerifyFinal(md_ctx, buffer, src.length());
    if (0 == err) {
      return 1;  // Verification failed, but no error occurred

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

int Certificate::verify_signature(
  const DDS::OctetSeq& src,
  const std::vector<const DDS::OctetSeq*>& expected_contents) const
{
#ifdef OPENSSL_V_1_0
  struct EVP_PKEY_Handle {
    EVP_PKEY* pkey_;
    explicit EVP_PKEY_Handle(EVP_PKEY* pkey) : pkey_(pkey) {}
    operator EVP_PKEY*() { return pkey_; }
    ~EVP_PKEY_Handle() { EVP_PKEY_free(pkey_); }
  } pkey(X509_get_pubkey(x_));
#else
  EVP_PKEY* pkey = X509_get0_pubkey(x_);
#endif
  verify_implementation verify(pkey);
  return verify(src, expected_contents);
}

int Certificate::subject_name_to_str(std::string& dst,
                                     unsigned long flags) const
{
  int result = 1;

  dst.clear();

  if (x_) {
    /* Do not free name! */
    X509_NAME* name = X509_get_subject_name(x_);
    if (name) {
      BIO* buffer = BIO_new(BIO_s_mem());
      if (buffer) {
        int len = X509_NAME_print_ex(buffer, name, 0, flags);
        if (len > 0) {
          std::vector<char> tmp(len +
                                1);  // BIO_gets will add null hence +1
          len = BIO_gets(buffer, &tmp[0], len + 1);
          if (len > 0) {
            std::copy(
                      tmp.begin(),
                      tmp.end() -
                      1,  // But... string inserts a null so it's not needed
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

  if (!x_) return 1;

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

const char* Certificate::keypair_algo() const
{
  // This should probably be pulling the information directly from
  // the certificate.
  if (std::string("RSASSA-PSS-SHA256") == dsign_algo_) {
    return "RSA-2048";

  } else if (std::string("ECDSA-SHA256") == dsign_algo_) {
    return "EC-prime256v1";

  } else {
    return "UNKNOWN";
  }
}

struct cache_dsign_algo_impl
{
  cache_dsign_algo_impl() : pkey_(NULL), rsa_(NULL), ec_(NULL) {}
  ~cache_dsign_algo_impl()
  {
    EVP_PKEY_free(pkey_);
    RSA_free(rsa_);
    EC_KEY_free(ec_);
  }

  int operator() (X509* cert, std::string& dst)
  {
    if (!cert) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) SSL::Certificate::cache_dsign_algo: WARNING, failed to "
                 "get pubkey from X509 cert\n"));
      return 1;
    }

    pkey_ = X509_get_pubkey(cert);
    if (!pkey_) {
      OPENDDS_SSL_LOG_ERR("cache_dsign_algo_impl::operator(): x509_get_pubkey failed");
      return 1;
    }

    rsa_ = EVP_PKEY_get1_RSA(pkey_);
    if (rsa_) {
      dst = "RSASSA-PSS-SHA256";
      return 0;
    }

    ec_ = EVP_PKEY_get1_EC_KEY(pkey_);
    if (ec_) {
      dst = "ECDSA-SHA256";
      return 0;
    }

    ACE_ERROR((LM_WARNING,
               "(%P|%t) SSL::Certificate::cache_dsign_algo: WARNING, only RSASSA-PSS-SHA256 or "
               "ECDSA-SHA256 are currently supported signature/verification algorithms\n"));

    return 1;
  }

private:
  EVP_PKEY* pkey_;
  RSA* rsa_;
  EC_KEY* ec_;
};

int Certificate::cache_dsign_algo()
{
  return cache_dsign_algo_impl()(x_, dsign_algo_);
}

void Certificate::load_cert_bytes(const std::string& path)
{
  std::ifstream in(path.c_str(), std::ios::binary);

  if (!in) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) Certificate::load_cert_bytes:"
               "WARNING: Failed to load file '%C'; '%m'\n",
               path.c_str()));
    return;
  }

  DCPS::SequenceBackInsertIterator<DDS::OctetSeq> back_inserter(original_bytes_);

  std::copy((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>(),
            back_inserter);

  // To appease the other DDS security implementations which
  // append a null byte at the end of the cert.
  *back_inserter = 0u;
}

void Certificate::load_cert_data_bytes(const std::string& data)
{
  // The minus 1 is because path contains a comma in element 0 and that
  // comma is not included in the cert string
  original_bytes_.length(static_cast<unsigned int>(data.size() - 1));
  std::memcpy(original_bytes_.get_buffer(), &data[1],
              original_bytes_.length());

  // To appease the other DDS security implementations which
  // append a null byte at the end of the cert.
  original_bytes_.length(original_bytes_.length() + 1);
  original_bytes_[original_bytes_.length() - 1] = 0;
}

X509* Certificate::x509_from_pem(const std::string& path,
                                 const std::string& password)
{
  X509* result = NULL;

  BIO* filebuf = BIO_new_file(path.c_str(), "r");
  if (filebuf) {
    if (password != "") {
      result =
        PEM_read_bio_X509_AUX(filebuf, NULL, NULL, (void*)password.c_str());
      if (!result) {
        OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
      }

    } else {
      result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL, NULL);
      if (!result) {
        OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
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

X509* Certificate::x509_from_pem(const DDS::OctetSeq& bytes,
                                 const std::string& password)
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
        result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL,
                                       (void*)password.c_str());
        if (!result) {
          OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
          break;
        }

      } else {
        result = PEM_read_bio_X509_AUX(filebuf, NULL, NULL, NULL);
        if (!result) {
          OPENDDS_SSL_LOG_ERR("PEM_read_bio_X509_AUX failed");
          break;
        }
      }

    } else {
      std::stringstream errmsg;
      errmsg << "BIO_new failed";
      OPENDDS_SSL_LOG_ERR(errmsg.str().c_str());
      break;
    }

  } while (0);

  BIO_free(filebuf);

  return result;
}

int Certificate::serialize(DDS::OctetSeq& dst) const
{
  std::copy(DCPS::const_sequence_begin(original_bytes_),
            DCPS::const_sequence_end(original_bytes_),
            DCPS::back_inserter(dst));

  if (dst.length() == original_bytes_.length()) {
    return 0;
  }

  return 1;
}

struct deserialize_impl
{
  explicit deserialize_impl(const DDS::OctetSeq& src)
    : src_(src), buffer_(BIO_new(BIO_s_mem()))
  {}

  ~deserialize_impl()
  {
    BIO_free(buffer_);
  }

  int operator() (X509*& dst)
  {
    if (dst) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) SSL::Certificate::deserialize: WARNING, an X509 certificate "
                 "has already been loaded\n"));
      return 1;
    }

    if (0 == src_.length()) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) SSL::Certificate::deserialize: WARNING, source OctetSeq contains no data"));
      return 1;
    }

    if (!buffer_) {
      OPENDDS_SSL_LOG_ERR("failed to allocate buffer with BIO_new");
      return 1;
    }

    const int len = BIO_write(buffer_, src_.get_buffer(), src_.length());
    if (len <= 0) {
      OPENDDS_SSL_LOG_ERR("failed to write OctetSeq to BIO");
      return 1;
    }

    dst = PEM_read_bio_X509_AUX(buffer_, NULL, NULL, NULL);
    if (! dst) {
      OPENDDS_SSL_LOG_ERR("failed to read X509 from BIO");
      return 1;
    }

    return 0;
  }

private:
  const DDS::OctetSeq& src_;
  BIO* buffer_;
};

int Certificate::deserialize(const DDS::OctetSeq& src)
{
  int err = deserialize_impl(src)(x_) || cache_dsign_algo();
  if (!err) {
    original_bytes_ = src;
  }

  return err;
}

std::ostream& operator<<(std::ostream& lhs, const Certificate& rhs)
{
  if (rhs.x_) {
    lhs << "Certificate: { is_ca? '"
        << (X509_check_ca(rhs.x_) ? "yes" : "no") << "'; }";

  } else {
    lhs << "NULL";
  }
  return lhs;
}

bool operator==(const Certificate& lhs, const Certificate& rhs)
{
  if (lhs.x_ && rhs.x_) {
    return (0 == X509_cmp(lhs.x_, rhs.x_)) &&
      (lhs.original_bytes_ == rhs.original_bytes_);
  }
  return (lhs.x_ == rhs.x_) &&
    (lhs.original_bytes_ == rhs.original_bytes_);
}

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

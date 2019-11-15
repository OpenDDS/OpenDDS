/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "SignedDocument.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/SequenceIterator.h"
#include "Err.h"
#include <openssl/pem.h>
#include <cstring>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <fstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

SignedDocument::SignedDocument(const std::string& uri)
  : doc_(NULL), original_(), verifiable_("")
{
  DDS::Security::SecurityException ex;
  if (!load(uri, ex)) {
    ACE_ERROR((LM_WARNING, "(%P|%t) %C\n", ex.message.in()));
  }
}

SignedDocument::SignedDocument()
  : doc_(NULL), original_(), verifiable_("")
{
}

SignedDocument::SignedDocument(const DDS::OctetSeq& src)
  : doc_(NULL), original_(), verifiable_("")
{
  deserialize(src);
}

SignedDocument::SignedDocument(const SignedDocument& rhs)
  : doc_(NULL), original_(), verifiable_("")
{
  if (0 < rhs.original_.length()) {
    deserialize(rhs.original_);
  }
}

SignedDocument::~SignedDocument()
{
  if (doc_) {
    PKCS7_free(doc_);
  }
}

SignedDocument& SignedDocument::operator=(const SignedDocument& rhs)
{
  if (this != &rhs) {
    if (0 < rhs.original_.length()) {
      deserialize(rhs.original_);
    }
  }
  return *this;
}

bool SignedDocument::load(const std::string& uri, DDS::Security::SecurityException& ex)
{
  using namespace CommonUtilities;

  if (doc_) {
    set_security_error(ex, -1, 0, "SSL::SignedDocument::load: WARNING: document already loaded");
    return false;
  }

  URI uri_info(uri);

  switch (uri_info.scheme) {
  case URI::URI_FILE:
    PKCS7_from_SMIME_file(uri_info.everything_else);
    break;

  case URI::URI_DATA:
    deserialize(uri_info.everything_else);
    break;

  case URI::URI_PKCS11:
  case URI::URI_UNKNOWN:
  default:
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SSL::SignedDocument::load: WARNING: Unsupported URI scheme\n"));
    break;
  }

  if (!loaded()) {
    std::stringstream msg;
    msg << "SSL::SignedDocument::load: WARNING: Failed to load document supplied "
      "with URI '"  << uri << "'";
    set_security_error(ex, -1, 0, msg.str().c_str());
    return false;
  }

  return true;
}

void SignedDocument::get_original(std::string& dst) const
{
  dst = "";

  std::copy(DCPS::const_sequence_begin(original_),
            DCPS::const_sequence_end(original_),
            std::back_inserter(dst));
}

bool SignedDocument::get_original_minus_smime(std::string& dst) const
{
  const std::string start_str("Content-Type: text/plain"), end_str("dds>");

  get_original(dst);

  const size_t found_begin = dst.find(start_str);

  if (found_begin != std::string::npos) {
    dst.erase(0, found_begin + start_str.length());

    const char* t = " \t\n\r\f\v";

    dst.erase(0, dst.find_first_not_of(t));
  } else {
    return false;
  }

  const size_t found_end = dst.find(end_str);

  if (found_end != std::string::npos) {
    dst.erase(found_end + end_str.length());
  } else {
    return false;
  }

  return true;
}

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

    const size_t len = BIO_write(reader_, content_.c_str(),
                                 static_cast<unsigned int>(content_.size()));
    if (len <= 0) {
      OPENDDS_SSL_LOG_ERR("BIO_write failed");
      return 1;
    }

    if (1 != PKCS7_verify(doc_, 0, store_, reader_, 0, flags)) {
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
  verify_signature_impl verify(doc_, verifiable_);
  return verify(ca);
}

int SignedDocument::serialize(DDS::OctetSeq& dst) const
{
  std::copy(DCPS::const_sequence_begin(original_),
            DCPS::const_sequence_end(original_),
            DCPS::back_inserter(dst));

  return dst.length() == original_.length() ? 0 : 1;
}

int SignedDocument::deserialize(const DDS::OctetSeq& src)
{
  if (doc_) {
    ACE_ERROR((LM_WARNING, "(%P|%t) SSL::Certificate::deserialize: WARNING, an X509 certificate has already been loaded\n"));
    return 1;
  }

  // Assume the trailing null is already set

  std::copy(DCPS::const_sequence_begin(src),
            DCPS::const_sequence_end(src),
            DCPS::back_inserter(original_));

  if (0 < original_.length()) {
    PKCS7_from_data(original_);
  }

  return 0;
}

int SignedDocument::deserialize(const std::string& src)
{
  if (doc_) {
    ACE_ERROR((LM_WARNING, "(%P|%t) SSL::Certificate::deserialize: WARNING, an X509 certificate has already been loaded\n"));
    return 1;
  }

  DCPS::SequenceBackInsertIterator<DDS::OctetSeq> back_inserter(original_);
  std::copy(src.begin(), src.end(), back_inserter);

  // To appease the other DDS security implementations
  *back_inserter = 0u;

  if (0 < original_.length()) {
    PKCS7_from_data(original_);
    if (! doc_) {
      return 1;
    }
  }
  return 0;
}

int SignedDocument::cache_verifiable(BIO* from)
{
  if (!doc_ || !from) {
    return 1;
  }

  unsigned char tmp[32] = { 0 };
  int len = 0;
  while ((len = BIO_read(from, tmp, sizeof(tmp))) > 0) {
    verifiable_.insert(verifiable_.end(), tmp, tmp + len);
  }

  if (0 < verifiable_.length()) {
    return 0;
  }

  return 1;
}

PKCS7* SignedDocument::PKCS7_from_SMIME_file(const std::string& path)
{
  std::ifstream in(path.c_str(), std::ios::binary);

  DCPS::SequenceBackInsertIterator<DDS::OctetSeq> back_inserter(original_);

  std::copy(std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>(),
            back_inserter);

  if (!in) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SignedDocument::PKCS7_from_SMIME_file:"
               "WARNING: Failed to load file '%C'; '%m'\n",
               path.c_str()));
    return NULL;
  }

  // To appease the other DDS security implementations
  *back_inserter = 0u;

  return original_.length() ? PKCS7_from_data(original_) : 0;
}

PKCS7* SignedDocument::PKCS7_from_data(const DDS::OctetSeq& s_mime_data)
{
  if (doc_) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SignedDocument::PKCS7_from_data: "
               "WARNING: document has already been constructed\n"));
    return NULL;
  }

  BIO* filebuf = BIO_new(BIO_s_mem());

  if (filebuf) {
    if (0 >= BIO_write(filebuf, s_mime_data.get_buffer(),
                       s_mime_data.length())) {
      OPENDDS_SSL_LOG_ERR("BIO_write failed");
    }

    BIO* cache_this = NULL;

    doc_ = SMIME_read_PKCS7(filebuf, &cache_this);

    if (!doc_) {
      OPENDDS_SSL_LOG_ERR("SMIME_read_PKCS7 failed");
    }

    if (0 != cache_verifiable(cache_this)) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) SignedDocument::PKCS7_from_data: "
                 "WARNING: failed to cache verifiable part of S/MIME data\n"));
    }

    BIO_free(filebuf);
    BIO_free(cache_this);

  } else {
    OPENDDS_SSL_LOG_ERR("failed to create data using BIO_new");
  }

  return doc_;
}

bool operator==(const SignedDocument& lhs, const SignedDocument& rhs)
{
  return lhs.original_ == rhs.original_ && lhs.verifiable_ == rhs.verifiable_;
}

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

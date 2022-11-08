/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "SignedDocument.h"

#include "Err.h"

#include <dds/DCPS/security/CommonUtilities.h>

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

namespace {
  const char* const default_filename = "<no filename: not loaded>";
  const char* const data_filename = "<no filename: data uri>";
}

SignedDocument::SignedDocument()
  : original_()
  , content_()
  , verified_(false)
  , filename_(default_filename)
{
}

SignedDocument::SignedDocument(const DDS::OctetSeq& src)
  : original_(src)
  , content_()
  , verified_(false)
  , filename_(data_filename)
{
}

SignedDocument::SignedDocument(const SignedDocument& rhs)
  : original_(rhs.original_)
  , content_(rhs.content_)
  , verified_(rhs.verified_)
  , filename_(rhs.filename_)
{
}

SignedDocument::~SignedDocument()
{
}

SignedDocument& SignedDocument::operator=(const SignedDocument& rhs)
{
  if (this != &rhs) {
    original_ = rhs.original_;
    content_ = rhs.content_;
    verified_ = rhs.verified_;
    filename_ = rhs.filename_;
  }
  return *this;
}

bool SignedDocument::load(const std::string& uri, DDS::Security::SecurityException& ex)
{
  using namespace CommonUtilities;

  original_.length(0);
  content_.clear();
  verified_ = false;
  filename_ = default_filename;

  URI uri_info(uri);

  switch (uri_info.scheme) {
  case URI::URI_FILE: {
    load_file(uri_info.everything_else);
    break;
  }
  case URI::URI_DATA:
    original_.length(static_cast<unsigned int>(uri_info.everything_else.length() + 1));
    std::memcpy(original_.get_buffer(), uri_info.everything_else.c_str(), uri_info.everything_else.length() + 1);
    filename_ = data_filename;
    break;

  case URI::URI_PKCS11:
  case URI::URI_UNKNOWN:
  default:
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SSL::SignedDocument::load: WARNING: Unsupported URI scheme\n"));
    break;
  }

  if (original_.length() == 0) {
    std::stringstream msg;
    msg << "SSL::SignedDocument::load: WARNING: Failed to load document supplied "
      "with URI '"  << uri << "'";
    set_security_error(ex, -1, 0, msg.str().c_str());
    return false;
  }

  return true;
}

bool SignedDocument::verify(const Certificate& ca)
{
  content_.clear();
  verified_ = false;

  X509_STORE* store = X509_STORE_new();
  if (store) {
    if (X509_STORE_add_cert(store, ca.x509()) == 1) {
      BIO* filebuf = BIO_new(BIO_s_mem());
      if (filebuf) {
        if (BIO_write(filebuf, original_.get_buffer(), original_.length()) > 0) {
          BIO* bcont = 0;
          PKCS7* doc = SMIME_read_PKCS7(filebuf, &bcont);
          if (doc) {
            BIO* content = BIO_new(BIO_s_mem());
            if (content) {
              if (PKCS7_verify(doc, 0, store, bcont, content, PKCS7_TEXT) == 1) {
                char* p = 0;
                long size = BIO_get_mem_data(content, &p);
                if (size >= 0) {
                  content_ = std::string(p, size);
                  verified_ = true;
                } else {
                  OPENDDS_SSL_LOG_ERR("BIO_get_mem_data failed");
                }
              } else {
                OPENDDS_SSL_LOG_ERR("PKCS7_verify failed");
              }
              BIO_free(content);
            } else {
              OPENDDS_SSL_LOG_ERR("BIO_new failed");
            }
            if (bcont) {
              BIO_free(bcont);
            }
            PKCS7_free(doc);
          } else {
            OPENDDS_SSL_LOG_ERR("SMIME_read_PKCS7 failed");
          }
        } else {
          OPENDDS_SSL_LOG_ERR("BIO_write failed");
        }
        BIO_free(filebuf);
      } else {
        OPENDDS_SSL_LOG_ERR("BIO_new failed");
      }
    } else {
      OPENDDS_SSL_LOG_ERR("X509_STORE_add_cert failed");
    }
    X509_STORE_free(store);
  } else {
    OPENDDS_SSL_LOG_ERR("X509_STORE_new failed");
  }

  return verified_;
}

void SignedDocument::load_file(const std::string& path)
{
  filename_ = path;

#ifdef ACE_ANDROID
  CORBA::Octet *buffer;

  char b[1024];
  FILE* fp = ACE_OS::fopen(path.c_str(), "rb");

  int n;
  int i = 0;
  while (!feof(fp)) {
    n = ACE_OS::fread(&b, 1, 1024, fp);
    i += n;

    original_.length(i + 1); // +1 for null byte at end of cert
    buffer = original_.get_buffer();
    ACE_OS::memcpy(buffer + i - n, b, n);
  }

  ACE_OS::fclose(fp);

  // To appease the other DDS security implementations which
  // append a null byte at the end of the cert.
  buffer[i + 1] = 0u;

#else
  std::ifstream in(path.c_str(), std::ios::binary);

  if (!in) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SignedDocument::PKCS7_from_SMIME_file:"
               "WARNING: Failed to load file '%C'; '%m'\n",
               path.c_str()));
    return;
  }

  const std::ifstream::pos_type begin = in.tellg();
  in.seekg(0, std::ios::end);
  const std::ifstream::pos_type end = in.tellg();
  in.seekg(0, std::ios::beg);

  original_.length(static_cast<CORBA::ULong>(end - begin + 1));
  in.read(reinterpret_cast<char*>(original_.get_buffer()), end - begin);

  if (!in) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) SignedDocument::PKCS7_from_SMIME_file:"
               "WARNING: Failed to load file '%C'; '%m'\n",
               path.c_str()));
    return;
  }

  // To appease the other DDS security implementations
  original_[original_.length() - 1] = 0u;
#endif
}

bool SignedDocument::operator==(const SignedDocument& other) const
{
  return original_ == other.original_ && content_ == other.content_ && verified_ == other.verified_;
}

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

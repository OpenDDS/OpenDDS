/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "LocalCredentialData.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/iterator_adaptor.h"
#include "dds/DCPS/debug.h"
#include <cstdio>
#include <cstring>
#include <cerrno>

namespace OpenDDS {
namespace Security {

int CredentialHash::operator()(DDS::OctetSeq& dst) const
{
  const DDS::OctetSeq& perm_data = permissions_data_;
  const DDS::OctetSeq& topic_data = participant_topic_data_;

  DDS::BinaryPropertySeq hash_data;
  DCPS::sequence_back_insert_iterator<DDS::BinaryPropertySeq> inserter(hash_data);

  DDS::BinaryProperty_t cid, cperm, cpdata, cdsign_algo, ckagree_algo;

  cid.name = "c.id";
  cid.value = pubcert_.original_bytes();
  cid.propagate = true;

  cperm.name = "c.perm";
  cperm.value = perm_data;
  cperm.propagate = true;

  cpdata.name = "c.pdata";
  cpdata.value = topic_data;
  cpdata.propagate = true;

  cdsign_algo.name = "c.dsign_algo";
  const char* cdsign_algo_str = pubcert_.dsign_algo();
  cdsign_algo.value.length(std::strlen(cdsign_algo_str) + 1);
  std::memcpy(cdsign_algo.value.get_buffer(), cdsign_algo_str, cdsign_algo.value.length());
  cdsign_algo.propagate = true;

  ckagree_algo.name = "c.kagree_algo";
  const char* ckagree_algo_str = dh_.kagree_algo();
  ckagree_algo.value.length(std::strlen(ckagree_algo_str) + 1);
  std::memcpy(ckagree_algo.value.get_buffer(), ckagree_algo_str, ckagree_algo.value.length());
  ckagree_algo.propagate = true;

  *inserter = cid;
  *inserter = cperm;
  *inserter = cpdata;
  *inserter = cdsign_algo;
  *inserter = ckagree_algo;

  return SSL::hash_serialized(hash_data, dst);
}

LocalAuthCredentialData::LocalAuthCredentialData(const DDS::PropertySeq& props)
{
  load(props);
}

LocalAuthCredentialData::LocalAuthCredentialData()
{

}

LocalAuthCredentialData::~LocalAuthCredentialData()
{

}

void LocalAuthCredentialData::load(const DDS::PropertySeq& props)
{
  using namespace CommonUtilities;

  std::string name, value, pkey_uri, password;
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT(
      "(%P|%t) LocalAuthCredentialData::load: Number of Properties: %i\n"
    ), props.length()));
  }
  for (size_t i = 0; i < props.length(); ++i) {
    name = props[i].name;
    value = props[i].value;

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT(
        "(%P|%t) LocalAuthCredentialData::load: property %i: %C: %C\n"
      ), i, name.c_str(), value.c_str()));
    }

    if (name == "dds.sec.auth.identity_ca") {
        ca_cert_.reset(new SSL::Certificate(value));

    } else if (name == "dds.sec.auth.private_key") {
        pkey_uri = value;

    } else if (name == "dds.sec.auth.identity_certificate") {
        participant_cert_.reset(new SSL::Certificate(value));

    } else if (name == "dds.sec.auth.password") {
        password = value;

    } else if (name == "dds.sec.access.permissions") {

      URI uri_info(value);

      switch(uri_info.scheme) {
        case URI::URI_FILE:
          load_permissions_file(uri_info.everything_else);
          break;

        case URI::URI_DATA:
          load_permissions_data(uri_info.everything_else);
          break;

        case URI::URI_PKCS11:
        case URI::URI_UNKNOWN:
        default:
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) LocalAuthCredentialData::load: ")
                     ACE_TEXT("ERROR: unsupported URI scheme in path '%C'\n"),
                     value.c_str()));

          break;
      }
    }
  }

  if (pkey_uri != "") {
    participant_pkey_.reset(new SSL::PrivateKey(pkey_uri, password));
  }
}

void LocalAuthCredentialData::load_permissions_file(const std::string& path)
{
  std::vector<unsigned char> chunks;
  unsigned char chunk[32] = {0};

  FILE* fp = fopen(path.c_str(), "rb");
  if (fp) {
    size_t count = 0u;
    while((count = fread(chunk, sizeof(chunk[0]), sizeof(chunk), fp))) {
      chunks.insert(chunks.end(),
                    chunk,
                    chunk + count);
    }

    if (ferror(fp)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) LocalAuthCredentialData::load_permissions_file: ")
                   ACE_TEXT("ERROR: '%C' occured while reading file '%C'\n"),
                   std::strerror(errno),
                   path.c_str()));

    } else {
        access_permissions_.length(chunks.size());
        std::memcpy(access_permissions_.get_buffer(), &chunks[0], access_permissions_.length());
    }

    // To appease the other DDS security implementations which
    // append a null byte at the end of the cert.
    access_permissions_.length(access_permissions_.length() + 1);
    access_permissions_[access_permissions_.length() - 1] = 0;

    fclose(fp);
  }
}

void LocalAuthCredentialData::load_permissions_data(const std::string& path)
{
  // The minus 1 is because path contains a comma in element 0 and that comma
  // is not included in the cert string
  access_permissions_.length(path.size() - 1);
  std::memcpy(access_permissions_.get_buffer(), &path[1], access_permissions_.length());

  // To appease the other DDS security implementations which
  // append a null byte at the end of the cert.
  access_permissions_.length(access_permissions_.length() + 1);
  access_permissions_[access_permissions_.length() - 1] = 0;
}

}
}

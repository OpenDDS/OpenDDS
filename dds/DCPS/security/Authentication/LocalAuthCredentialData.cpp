/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "LocalAuthCredentialData.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/security/TokenReader.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/SequenceIterator.h"
#include <algorithm>
#include <cstring>
#include <cerrno>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

using namespace CommonUtilities;

int CredentialHash::operator()(DDS::OctetSeq& dst) const
{
  const DDS::OctetSeq& perm_data = permissions_data_;
  const DDS::OctetSeq& topic_data = participant_topic_data_;

  DDS::BinaryPropertySeq hash_data;
  DCPS::SequenceBackInsertIterator<DDS::BinaryPropertySeq> inserter(hash_data);

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

LocalAuthCredentialData::LocalAuthCredentialData()
{
}

LocalAuthCredentialData::~LocalAuthCredentialData()
{
}

bool LocalAuthCredentialData::load_access_permissions(const DDS::Security::PermissionsCredentialToken& src,
                                                      DDS::Security::SecurityException& ex)
{
  const char* cperm = TokenReader(src).get_property_value("dds.perm.cert");
  if (!cperm) {
    set_security_error(ex, -1, 0,
                       "LocalAuthCredentialData::load_access_permissions: "
                       "no 'dds.perm.cert' property provided");
    return false;
  }

  DCPS::SequenceBackInsertIterator<DDS::OctetSeq> back_inserter(access_permissions_);
  std::copy(cperm, cperm + std::strlen(cperm), back_inserter);

  *back_inserter = 0u;
  return true;
}

bool LocalAuthCredentialData::load_credentials(const DDS::PropertySeq& props, DDS::Security::SecurityException& ex)
{
  std::string name, value, pkey_uri, password;
  if (DCPS::DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) LocalAuthCredentialData::load: Number of Properties: %i\n", props.length()));
  }
  for (size_t i = 0; i < props.length(); ++i) {
    name = props[i].name;
    value = props[i].value;

    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) LocalAuthCredentialData::load: property %i: %C: %C\n",
                 int(i), name.c_str(), value.c_str()));
    }

    if (name == "dds.sec.auth.identity_ca") {
      ca_cert_.reset(new SSL::Certificate(value));

    } else if (name == "dds.sec.auth.private_key") {
      pkey_uri = value;

    } else if (name == "dds.sec.auth.identity_certificate") {
      participant_cert_.reset(new SSL::Certificate(value));

    } else if (name == "dds.sec.auth.password") {
      password = value;

    }
  }

  if (!pkey_uri.empty()) {
    participant_pkey_.reset(new SSL::PrivateKey(pkey_uri, password));
  }

  if (!ca_cert_) {
    set_security_error(ex, -1, 0, "LocalAuthCredentialData::load: failed to load CA certificate");
    return false;

  } else if (!participant_cert_) {
    set_security_error(ex, -1, 0, "LocalAuthCredentialData::load: failed to load participant certificate");
    return false;

  } else if (!participant_pkey_) {
    set_security_error(ex, -1, 0, "LocalAuthCredentialData::load: failed to load participant private key");
    return false;
  }

  return true;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

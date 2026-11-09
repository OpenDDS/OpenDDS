#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY

#include <dds/DCPS/security/framework/HandleRegistry.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::Security;
using DDS::Security::EndpointSecurityAttributes;

namespace {

GUID_t make_test_guid(CORBA::Octet kind)
{
  GUID_t guid = GUID_UNKNOWN;
  guid.guidPrefix[0] = 1;
  guid.entityId.entityKey[2] = 1;
  guid.entityId.entityKind = kind;
  return guid;
}

template <typename Insert, typename Get, typename Erase>
void check_attributes_outlive_erase(CORBA::Octet kind, Insert insert, Get get, Erase erase)
{
  HandleRegistry_rch registry = make_rch<HandleRegistry>();
  const GUID_t id = make_test_guid(kind);
  EndpointSecurityAttributes inserted = registry->default_endpoint_security_attributes();
  inserted.is_submessage_protected = true;
  inserted.is_payload_protected = true;
  ((*registry).*insert)(id, 1, inserted);

  // Callers use the result after the lock is released, so it must survive an erase.
  const EndpointSecurityAttributes& attributes = ((*registry).*get)(id);
  ((*registry).*erase)(id);
  EXPECT_TRUE(attributes.is_submessage_protected);
  EXPECT_TRUE(attributes.is_payload_protected);

  EXPECT_FALSE(((*registry).*get)(id).is_submessage_protected);
}

}

TEST(dds_DCPS_security_framework_HandleRegistry, local_datareader_security_attributes_outlive_erase)
{
  check_attributes_outlive_erase(ENTITYKIND_USER_READER_WITH_KEY,
                                 &HandleRegistry::insert_local_datareader_crypto_handle,
                                 &HandleRegistry::get_local_datareader_security_attributes,
                                 &HandleRegistry::erase_local_datareader_crypto_handle);
}

TEST(dds_DCPS_security_framework_HandleRegistry, local_datawriter_security_attributes_outlive_erase)
{
  check_attributes_outlive_erase(ENTITYKIND_USER_WRITER_WITH_KEY,
                                 &HandleRegistry::insert_local_datawriter_crypto_handle,
                                 &HandleRegistry::get_local_datawriter_security_attributes,
                                 &HandleRegistry::erase_local_datawriter_crypto_handle);
}

TEST(dds_DCPS_security_framework_HandleRegistry, remote_datareader_security_attributes_outlive_erase)
{
  check_attributes_outlive_erase(ENTITYKIND_USER_READER_WITH_KEY,
                                 &HandleRegistry::insert_remote_datareader_crypto_handle,
                                 &HandleRegistry::get_remote_datareader_security_attributes,
                                 &HandleRegistry::erase_remote_datareader_crypto_handle);
}

TEST(dds_DCPS_security_framework_HandleRegistry, remote_datawriter_security_attributes_outlive_erase)
{
  check_attributes_outlive_erase(ENTITYKIND_USER_WRITER_WITH_KEY,
                                 &HandleRegistry::insert_remote_datawriter_crypto_handle,
                                 &HandleRegistry::get_remote_datawriter_security_attributes,
                                 &HandleRegistry::erase_remote_datawriter_crypto_handle);
}

#endif

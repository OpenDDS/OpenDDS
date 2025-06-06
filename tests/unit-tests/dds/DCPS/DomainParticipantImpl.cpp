#include <gtest/gtest.h>

#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"

#include <string.h>

typedef OpenDDS::DCPS::DomainParticipantImpl::RepoIdSequence RepoIdSequence;
typedef OpenDDS::DCPS::GUID_t GUID_t;

namespace {
  namespace Factory {
    GUID_t not_default_repo_id() {
      GUID_t result;
      int i = 1;
      for (int idx = 0; idx < 12; ++idx) {
        result.guidPrefix[idx] = static_cast<DDS::Byte>(i += 2);
      }
      result.entityId.entityKey[0] = 0;
      result.entityId.entityKey[1] = 0;
      result.entityId.entityKey[2] = 0;
      result.entityId.entityKind = 123;
      return result;
    }
  };

  long entityKey(const GUID_t& id) {
    return (((id.entityId.entityKey[0] << 8) | id.entityId.entityKey[1]) << 8) | id.entityId.entityKey[2];
  }
};

TEST(dds_DCPS_DomainParticipantImpl, maintest)
{
  { // First sequence should be 1
    GUID_t repoId;
    memset(&repoId, 0, sizeof(repoId));
    RepoIdSequence seq(repoId);
    EXPECT_TRUE(entityKey(seq.next()) == 1);
    // Subsequent sequences should increase
    EXPECT_TRUE(entityKey(seq.next()) == 2);
    EXPECT_TRUE(entityKey(seq.next()) == 3);
    EXPECT_TRUE(entityKey(seq.next()) == 4);
  }
  { // Should preserve GUID_t structure
    GUID_t repoId = Factory::not_default_repo_id();
    RepoIdSequence seq(repoId);

    {
      GUID_t result = seq.next();
      EXPECT_TRUE(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      EXPECT_TRUE(entityKey(result) == 1);
      EXPECT_TRUE(repoId.entityId.entityKind == result.entityId.entityKind);
    }
    {
      GUID_t result = seq.next();
      EXPECT_TRUE(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      EXPECT_TRUE(entityKey(result) == 2);
      EXPECT_TRUE(repoId.entityId.entityKind == result.entityId.entityKind);
    }
    {
      GUID_t result = seq.next();
      EXPECT_TRUE(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      EXPECT_TRUE(entityKey(result) == 3);
      EXPECT_TRUE(repoId.entityId.entityKind == result.entityId.entityKind);
    }
  }
}

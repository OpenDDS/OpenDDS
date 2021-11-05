#include <gtest/gtest.h>

#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"

#include <string.h>

typedef OpenDDS::DCPS::DomainParticipantImpl::RepoIdSequence RepoIdSequence;
typedef OpenDDS::DCPS::RepoId RepoId;

namespace {
  namespace Factory {
    RepoId not_default_repo_id() {
      RepoId result;
      int i = 1;
      result.guidPrefix[ 0] =  i += 2;
      result.guidPrefix[ 1] =  i += 2;
      result.guidPrefix[ 2] =  i += 2;
      result.guidPrefix[ 3] =  i += 2;
      result.guidPrefix[ 4] =  i += 2;
      result.guidPrefix[ 5] =  i += 2;
      result.guidPrefix[ 6] =  i += 2;
      result.guidPrefix[ 7] =  i += 2;
      result.guidPrefix[ 8] =  i += 2;
      result.guidPrefix[ 9] =  i += 2;
      result.guidPrefix[10] =  i += 2;
      result.guidPrefix[11] =  i += 2;
      result.entityId.entityKey[0] = 0;
      result.entityId.entityKey[1] = 0;
      result.entityId.entityKey[2] = 0;
      result.entityId.entityKind = 123;
      return result;
    }
  };

  long entityKey(const RepoId& id) {
    long result = (((id.entityId.entityKey[0]  << 8) |
                     id.entityId.entityKey[1]) << 8) |
                     id.entityId.entityKey[2];
    return result;
  }
};

TEST(dds_DCPS_DomainParticipantImpl, maintest)
{
  { // First sequence should be 1
    RepoId repoId;
    memset(&repoId, 0, sizeof(repoId));
    RepoIdSequence seq(repoId);
    EXPECT_TRUE(entityKey(seq.next()) == 1);
    // Subsequent sequences should increase
    EXPECT_TRUE(entityKey(seq.next()) == 2);
    EXPECT_TRUE(entityKey(seq.next()) == 3);
    EXPECT_TRUE(entityKey(seq.next()) == 4);
  }
  { // Should preserve RepoId structure
    RepoId repoId = Factory::not_default_repo_id();
    RepoIdSequence seq(repoId);

    {
      RepoId result = seq.next();
      EXPECT_TRUE(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      EXPECT_TRUE(entityKey(result) == 1);
      EXPECT_TRUE(repoId.entityId.entityKind == result.entityId.entityKind);
    }
    {
      RepoId result = seq.next();
      EXPECT_TRUE(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      EXPECT_TRUE(entityKey(result) == 2);
      EXPECT_TRUE(repoId.entityId.entityKind == result.entityId.entityKind);
    }
    {
      RepoId result = seq.next();
      EXPECT_TRUE(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      EXPECT_TRUE(entityKey(result) == 3);
      EXPECT_TRUE(repoId.entityId.entityKind == result.entityId.entityKind);
    }
  }
}

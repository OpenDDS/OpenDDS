
#include "ace/OS_main.h"
#include "../common/TestSupport.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"

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

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  { // First sequence should be 1
    RepoId repoId;
    memset(&repoId, 0, sizeof(repoId));
    RepoIdSequence seq(repoId);
    TEST_ASSERT(entityKey(seq.next()) == 1);
    // Subsequent sequences should increase
    TEST_ASSERT(entityKey(seq.next()) == 2);
    TEST_ASSERT(entityKey(seq.next()) == 3);
    TEST_ASSERT(entityKey(seq.next()) == 4);
  }
  { // Should preserve RepoId structure
    RepoId repoId = Factory::not_default_repo_id();
    RepoIdSequence seq(repoId);

    {
      RepoId result = seq.next();
      TEST_ASSERT(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      TEST_ASSERT(entityKey(result) == 1);
      TEST_ASSERT(repoId.entityId.entityKind == result.entityId.entityKind);
    }
    {
      RepoId result = seq.next();
      TEST_ASSERT(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      TEST_ASSERT(entityKey(result) == 2);
      TEST_ASSERT(repoId.entityId.entityKind == result.entityId.entityKind);
    }
    {
      RepoId result = seq.next();
      TEST_ASSERT(!memcmp(repoId.guidPrefix,
                          result.guidPrefix,
                          sizeof repoId.guidPrefix));
      TEST_ASSERT(entityKey(result) == 3);
      TEST_ASSERT(repoId.entityId.entityKind == result.entityId.entityKind);
    }
  }
  return 0;
}

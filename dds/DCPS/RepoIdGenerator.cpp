/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "RepoIdGenerator.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "ace/Log_Msg.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

const unsigned int RepoIdGenerator::KeyBits = 24;

const unsigned int RepoIdGenerator::KeyMask = (1 << KeyBits) - 1;

RepoIdGenerator::RepoIdGenerator(long federation, long participant, EntityKind kind)
  : kind_(kind)
  , federation_(federation)
  , participant_(participant)
  , lastKey_(0)
{
}

RepoIdGenerator::~RepoIdGenerator()
{
}

RepoId
RepoIdGenerator::next()
{
  // Generate a new key value.
  ++lastKey_;

  RepoIdBuilder builder;
  builder.federationId(federation_);

  // Generate a Participant GUID value.
  if (kind_ == KIND_PARTICIPANT) {

    // Rudimentary validity checking.
    if (lastKey_ == 0) {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: RepoIdGenerator::next: ")
                 ACE_TEXT("Exceeded Maximum number of participant keys!")
                 ACE_TEXT("Next key will be a duplicate!\n")));
    }

    builder.participantId(lastKey_);
    builder.entityId(ENTITYID_PARTICIPANT);

    // Generate an Entity GUID value.

  } else {

    // Rudimentary validity checking.
    if ((lastKey_ & ~KeyMask) != 0) {
      // We have rolled over and there can now exist objects with
      // the same key.
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: RepoIdGenerator::next: ")
                 ACE_TEXT("Exceeded Maximum number of entity keys!")
                 ACE_TEXT("Next key will be a duplicate!\n")));
    }

    builder.participantId(participant_);
    builder.entityKey(lastKey_);
    builder.entityKind(kind_);
  }

  return RepoId(builder);
}

void
RepoIdGenerator::last(long key)
{
  if (key > lastKey_) {
    lastKey_ = key;
  }
}

} // namespace DCPS
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

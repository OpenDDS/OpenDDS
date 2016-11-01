/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE void
RepoIdBuilder::federationId(long federationId)
{
  guidPrefix1(federationId);
}

ACE_INLINE void
RepoIdBuilder::participantId(long participantId)
{
  guidPrefix2(participantId);
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

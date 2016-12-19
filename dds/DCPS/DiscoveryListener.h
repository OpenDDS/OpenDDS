/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DISCOVERYLISTENER_H
#define OPENDDS_DCPS_DISCOVERYLISTENER_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
* @class DiscoveryListener
*
* @brief Defines the interface that allows DataWriters (and lower levels) to inform discovery.
*
*/
class DiscoveryListener {
public:

  DiscoveryListener() {}

  virtual ~DiscoveryListener() {}

  virtual void reader_exists(const RepoId& readerid, const RepoId& writerid) = 0;
  virtual void reader_does_not_exist(const RepoId& readerid, const RepoId& writerid) = 0;
  virtual void writer_exists(const RepoId& writerid, const RepoId& readerid) = 0;
  virtual void writer_does_not_exist(const RepoId& writerid, const RepoId& readerid) = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DISCOVERYLISTENER_H  */

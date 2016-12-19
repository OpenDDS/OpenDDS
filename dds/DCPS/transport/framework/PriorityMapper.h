/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PRIORITY_MAPPER_H
#define PRIORITY_MAPPER_H

#include "dds/DCPS/dcps_export.h"

#include "TransportDefs.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class PriorityMapper
 *
 * @brief Encapsulate a TRANSPORT_PRIORITY value.
 *
 * This interface allows a TRANSPORT_PRIORIY value to be mapped to
 * values to use as DiffServ codepoint and thread priority values.
 * Specific implementations of this interface can define specific
 * mapping algorithms for conversion from the basic
 * TRANSPORT_PRIORITY value to the values to use for network and
 * thread prirorities.
 *
 * We take our cue from the RTCORBA code base in defining the types
 * of the priority values mapped to.  RTCORBA defines both the
 * network and thread priority values as shorts.  Since this is well
 * within the expected range of priority values to be mapped to and
 * the short value can be expanded into larger types without
 * complaint, this priority type should be without problem here.
 */
class OpenDDS_Dcps_Export PriorityMapper {
public:
  /// Construct with a priority value.
  PriorityMapper(Priority priority = 0);

  virtual ~PriorityMapper();

  /// Accessors for the TRANSPORT_PRIORITY value.
  Priority& priority();
  Priority  priority() const;

  /// Access the mapped DiffServ codepoint value.
  virtual short codepoint() const = 0;

  /// Access the mapped thread priority value.
  virtual short thread_priority() const = 0;

private:
  /// The TRANSPORT_PRIORITY value.
  Priority priority_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "PriorityMapper.inl"
#endif /* __ACE_INLINE__ */

#endif  /* PRIORITY_MAPPER_H */

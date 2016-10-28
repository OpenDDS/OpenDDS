/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_EXCEPTIONS_H
#define OPENDDS_DCPS_TRANSPORT_EXCEPTIONS_H

#include "dds/DCPS/dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
namespace Transport {

class OpenDDS_Dcps_Export Exception {
public:
  virtual ~Exception() {}
};

class OpenDDS_Dcps_Export NotFound       : public Exception {};
class OpenDDS_Dcps_Export Duplicate      : public Exception {};
class OpenDDS_Dcps_Export UnableToCreate : public Exception {};
class OpenDDS_Dcps_Export MiscProblem    : public Exception {};
class OpenDDS_Dcps_Export NotConfigured  : public Exception {};
class OpenDDS_Dcps_Export ConfigurationConflict  : public Exception {};

} // namespace Transport
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_TRANSPORT_EXCEPTIONS_H */

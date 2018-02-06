/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsSecurityCoreC.h"

#ifndef OPENDDS_SECURITY_WRAPPERS_H
#define OPENDDS_SECURITY_WRAPPERS_H

namespace OpenDDS {
  namespace Security {

    struct DiscoveredWriterData_SecurityWrapper {
      OpenDDS::DCPS::DiscoveredWriterData data;
      DDS::Security::EndpointSecurityInfo info;
      DDS::Security::DataTags tags;
    };

    struct DiscoveredReaderData_SecurityWrapper {
      OpenDDS::DCPS::DiscoveredReaderData data;
      DDS::Security::EndpointSecurityInfo info;
      DDS::Security::DataTags tags;
    };

  }
}

#endif

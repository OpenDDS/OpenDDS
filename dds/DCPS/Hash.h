/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_HASH_H
#define OPENDDS_DCPS_HASH_H

#include "dds/Versioned_Namespace.h"

#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

typedef unsigned char MD5Result[16];

OpenDDS_Dcps_Export
void MD5Hash(MD5Result& result, const void* input, size_t size);

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_HASH_H

/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_ACCESS_PERMISSIONS_H
#define OPENDDS_ACCESS_PERMISSIONS_H

#include "dds/DCPS/security/SSL/SignedDocument.h"
#include <string>

namespace OpenDDS {
namespace Security {

class Permissions
{
public:
  Permissions(const SSL::SignedDocument& doc);
private:
};

}
}

#endif

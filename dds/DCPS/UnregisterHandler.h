/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UNREGISTER_HANDLER_H
#define UNREGISTER_HANDLER_H

namespace OpenDDS { namespace DCPS {

class UnregisterHandler {
public:
  virtual DDS::ReturnCode_t unregister_instance_i(
    DDS::InstanceHandle_t handle,
    const DDS::Time_t & source_timestamp) = 0;
};

} }

#endif /* UNREGISTER_HANDLER_H */

/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NOP_DATA_WRITER_LISTENER_H
#define OPENDDS_DCPS_NOP_DATA_WRITER_LISTENER_H

#include "LocalObject.h"

#include <dds/Versioned_Namespace.h>

#include <dds/DdsDcpsPublicationC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class NopDataWriterListener : public virtual LocalObject<DDS::DataWriterListener> {
public:
  void on_offered_deadline_missed(
    DDS::DataWriter_ptr /*writer*/,
    const DDS::OfferedDeadlineMissedStatus& /*status*/)
  {
  }

  void on_offered_incompatible_qos(
    DDS::DataWriter_ptr /*writer*/,
    const DDS::OfferedIncompatibleQosStatus& /*status*/)
  {
  }

  void on_liveliness_lost(
    DDS::DataWriter_ptr /*writer*/,
    const DDS::LivelinessLostStatus& /*status*/)
  {
  }

  void on_publication_matched(
    DDS::DataWriter_ptr /*writer*/,
    const DDS::PublicationMatchedStatus& /*status*/)
  {
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_NOP_DATA_WRITER_LISTENER_H */

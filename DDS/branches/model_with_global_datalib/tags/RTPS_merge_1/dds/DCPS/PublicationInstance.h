/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PUBLICATION_INSTANCE_H
#define OPENDDS_DCPS_PUBLICATION_INSTANCE_H

#include "dcps_export.h"
#include "DataSampleList.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

/**
  * @class PublicationInstance
  *
  * @brief Struct that has information about an instance and the instance
  *        sample list.
  * @note  This struct retains the ownership of the registered sample data
  *        from typed datawriter. The data will be duplicated for the register,
  *        unregister and dispose control message.
  */
struct OpenDDS_Dcps_Export PublicationInstance {

  PublicationInstance(DataSample* registered_sample)
    : sequence_(),
      group_id_(0),
      registered_sample_(registered_sample),
      unregistered_(false),
      instance_handle_(0),
      deadline_timer_id_(-1) {
  }

  ~PublicationInstance() {
    // Release will decrement the reference count and
    // the sample data is released when the reference
    // count is 0.
    registered_sample_->release();
  }

  /// The sequence number.
  SequenceNumber   sequence_ ;

  /// The group id. // NOT USED IN FIRST IMPL
  CoherencyGroup   group_id_ ;

  /// The sample data for registration.
  DataSample*      registered_sample_;

  /// History of the instance samples.
  DataSampleList   samples_;

  /// The list of samples that wait for available space.
  DataSampleList   waiting_list_;

  /// The flag to indicate whether the instance is unregistered.
  bool             unregistered_;

  /// The instance handle for the registered object
  DDS::InstanceHandle_t instance_handle_;

  ACE_Time_Value   last_sample_tv_;

  ACE_Time_Value   cur_sample_tv_;

  long             deadline_timer_id_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_PUBLICATION_INSTANCE_H */

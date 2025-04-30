/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAWRITER_LISTENER_IMPL
#define DATAWRITER_LISTENER_IMPL

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <dds/DdsDcpsPublicationC.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

class DataWriterListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataWriterListener> {
public:
  DataWriterListenerImpl(DistributedConditionSet_rch dcs)
    : dcs_(dcs)
  {}

  virtual ~DataWriterListenerImpl() {}

  void on_offered_deadline_missed(DDS::DataWriter_ptr,
                                  const DDS::OfferedDeadlineMissedStatus&)
  {}

  void on_offered_incompatible_qos(DDS::DataWriter_ptr,
                                   const DDS::OfferedIncompatibleQosStatus&)
  {}

  void on_liveliness_lost(DDS::DataWriter_ptr,
                          const ::DDS::LivelinessLostStatus&)
  {}

  void on_publication_matched(DDS::DataWriter_ptr,
                              const DDS::PublicationMatchedStatus& status)
  {
    dcs_->post("Publisher", OpenDDS::DCPS::String("on_publication_matched")
               + "_" + OpenDDS::DCPS::to_dds_string(status.total_count)
               + "_" + OpenDDS::DCPS::to_dds_string(status.total_count_change)
               + "_" + OpenDDS::DCPS::to_dds_string(status.current_count)
               + "_" + OpenDDS::DCPS::to_dds_string(status.current_count_change));
  }

private:
  DistributedConditionSet_rch dcs_;
};

#endif /* DATAWRITER_LISTENER_IMPL  */

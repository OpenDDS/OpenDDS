/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "GroupRakeData.h"
#include "SubscriptionInstance.h"
#include "DataReaderImpl.h"
#include "QueryConditionImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

GroupRakeData::GroupRakeData()
{
}


bool GroupRakeData::insert_sample(ReceivedDataElement* sample,
                                  ReceivedDataElementList* rdel,
                                  SubscriptionInstance_rch instance,
                                  size_t index_in_instance)
{
  // Ignore DISPOSE and UNREGISTER messages in case they are sent
  // in the group coherent changes, but it shouldn't.
  if (!sample->valid_data_) {
    return false;
  }

  RakeData rd = {sample, rdel, instance, index_in_instance};
  this->sorted_.insert(rd);

  this->current_sample_ = this->sorted_.begin();
  return true;
}


void
GroupRakeData::get_datareaders(DDS::DataReaderSeq & readers)
{
  ACE_UNUSED_ARG(readers);
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  readers.length(static_cast<CORBA::ULong>(this->sorted_.size()));
  CORBA::ULong i = 0;
  SortedSet::iterator itEnd = this->sorted_.end();
  for (SortedSet::iterator it = this->sorted_.begin(); it != itEnd; ++it) {
    RcHandle<DDS::DataReader> reader = it->si_->instance_state_->data_reader().lock();
    if (reader) {
      readers[i++] = DDS::DataReader::_duplicate(reader.in());
    } else {
      readers.length(readers.length() - 1);
    }
  }
#endif
}


void
GroupRakeData::reset()
{
  this->sorted_.clear();
}


RakeData
GroupRakeData::get_data()
{
  RakeData data = *this->current_sample_;
  ++this->current_sample_;
  return data;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

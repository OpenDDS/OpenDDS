/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dds/DCPS/GroupRakeData.h"
#include "dds/DCPS/SubscriptionInstance.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/QueryConditionImpl.h"

namespace OpenDDS {
namespace DCPS {

GroupRakeData::GroupRakeData()
{
}


bool GroupRakeData::insert_sample(ReceivedDataElement* sample,
                                  SubscriptionInstance* instance,
                                  size_t index_in_instance)
{
  // Ignore DISPOSE and UNREGISTER messages in case they are sent
  // in the group coherent changes, but it shouldn't.
  if (! sample->registered_data_) return false;

  RakeData rd = {sample, instance, index_in_instance};
  sorted_.insert(rd);

  this->current_sample_ = sorted_.begin ();
  return true;
}



void
GroupRakeData::get_datareaders (DDS::DataReaderSeq & readers)
{
  readers.length (this->sorted_.size ());
  int i = 0;
  SortedSet::iterator itEnd = this->sorted_.end ();
  for (SortedSet::iterator it = this->sorted_.begin ();
    it != itEnd; ++it) {
    readers[i] = DDS::DataReader::_duplicate (it->si_->instance_state_.data_reader());
    ++i;
  }
}


void
GroupRakeData::reset ()
{
  this->sorted_.clear ();
}


RakeData
GroupRakeData::get_data ()
{
  RakeData data = *this->current_sample_;
  ++ this->current_sample_;
  return data;
}

} // namespace DCPS
} // namespace OpenDDS



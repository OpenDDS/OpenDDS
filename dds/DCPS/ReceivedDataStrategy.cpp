/*
 * $Id$
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Qos_Helper.h"
#include "ReceivedDataElementList.h"
#include "ReceivedDataStrategy.h"

namespace OpenDDS
{
namespace DCPS
{
ReceivedDataStrategy::ReceivedDataStrategy(
    ReceivedDataElementList& rcvd_samples)
  : rcvd_samples_(rcvd_samples)
{}

ReceivedDataStrategy::~ReceivedDataStrategy()
{}


ReceptionDataStrategy::ReceptionDataStrategy(
    ReceivedDataElementList& rcvd_samples)
  : ReceivedDataStrategy(rcvd_samples)
{}

ReceptionDataStrategy::~ReceptionDataStrategy()
{}

void
ReceptionDataStrategy::add(ReceivedDataElement* data_sample)
{
  this->rcvd_samples_.add(data_sample);
}


SourceDataStrategy::SourceDataStrategy(
    ReceivedDataElementList& rcvd_samples)
  : ReceivedDataStrategy(rcvd_samples)
{}

SourceDataStrategy::~SourceDataStrategy()
{}

void
SourceDataStrategy::add(ReceivedDataElement* data_sample)
{
  for (ReceivedDataElement* it = this->rcvd_samples_.head_;
       it != 0; it = it->next_data_sample_)
  {
    if (data_sample->source_timestamp_ < it->source_timestamp_)
    {
      data_sample->previous_data_sample_ = it->previous_data_sample_;
      data_sample->next_data_sample_ = it;

      // Are we replacing the head?
      if (it->previous_data_sample_ == 0)
      {
        this->rcvd_samples_.head_ = data_sample;
      }
      else
      {
	it->previous_data_sample_->next_data_sample_ = data_sample;
      }
      it->previous_data_sample_ = data_sample;

      ++this->rcvd_samples_.size_;

      return;
    }
  }

  this->rcvd_samples_.add(data_sample);
}

} // namespace
} // namespace

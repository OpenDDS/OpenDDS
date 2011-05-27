/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Qos_Helper.h"
#include "ReceivedDataElementList.h"
#include "ReceivedDataStrategy.h"

namespace {

class CoherentFilter : public OpenDDS::DCPS::ReceivedDataFilter {
public:
  bool operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample) {
    return data_sample->coherent_change_;
  }
};

class AcceptCoherent : public OpenDDS::DCPS::ReceivedDataOperation {
public:
  void operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample) {
    // Clear coherent_change_ flag; this makes
    // the data available for read/take operations.
    data_sample->coherent_change_ = false;
  }
};

} // namespace

namespace OpenDDS {
namespace DCPS {

ReceivedDataStrategy::ReceivedDataStrategy(
  ReceivedDataElementList& rcvd_samples)
  : rcvd_samples_(rcvd_samples)
{}

ReceivedDataStrategy::~ReceivedDataStrategy()
{}

void
ReceivedDataStrategy::add(ReceivedDataElement* data_sample)
{
  this->rcvd_samples_.add(data_sample);
}

void
ReceivedDataStrategy::accept_coherent()
{
  CoherentFilter    filter = CoherentFilter();
  AcceptCoherent operation = AcceptCoherent();
  this->rcvd_samples_.apply_all(filter, operation);
}

void
ReceivedDataStrategy::reject_coherent()
{
  CoherentFilter filter = CoherentFilter();
  this->rcvd_samples_.remove(filter, true);
}

ReceptionDataStrategy::ReceptionDataStrategy(
  ReceivedDataElementList& rcvd_samples)
  : ReceivedDataStrategy(rcvd_samples)
{}

ReceptionDataStrategy::~ReceptionDataStrategy()
{}

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
       it != 0; it = it->next_data_sample_) {
    if (data_sample->source_timestamp_ < it->source_timestamp_) {
      data_sample->previous_data_sample_ = it->previous_data_sample_;
      data_sample->next_data_sample_ = it;

      // Are we replacing the head?
      if (it->previous_data_sample_ == 0) {
        this->rcvd_samples_.head_ = data_sample;

      } else {
        it->previous_data_sample_->next_data_sample_ = data_sample;
      }

      it->previous_data_sample_ = data_sample;

      ++this->rcvd_samples_.size_;

      return;
    }
  }

  this->rcvd_samples_.add(data_sample);
}

} // namespace DCPS
} // namespace OpenDDS

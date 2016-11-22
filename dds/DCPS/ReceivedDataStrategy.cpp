/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Qos_Helper.h"
#include "ReceivedDataElementList.h"
#include "ReceivedDataStrategy.h"
#include "GuidUtils.h"

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

namespace {

class CoherentFilter : public OpenDDS::DCPS::ReceivedDataFilter {
public:
  CoherentFilter (OpenDDS::DCPS::PublicationId& writer,
                  OpenDDS::DCPS::RepoId& publisher)
  : writer_ (writer),
    publisher_ (publisher),
    group_coherent_ (! (this->publisher_ == ::OpenDDS::DCPS::GUID_UNKNOWN))
  {}

  bool operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample) {
    if (this->group_coherent_) {
      return data_sample->coherent_change_
             && (this->publisher_ == data_sample->publisher_id_);
    }
    else {
      return data_sample->coherent_change_
             && (this->writer_ == data_sample->pub_);
    }
  }

private:

  OpenDDS::DCPS::PublicationId& writer_;
  OpenDDS::DCPS::RepoId& publisher_;
  bool group_coherent_;
};

class AcceptCoherent : public OpenDDS::DCPS::ReceivedDataOperation {
public:
  AcceptCoherent (OpenDDS::DCPS::PublicationId& writer,
                  OpenDDS::DCPS::RepoId& publisher)
  : writer_ (writer),
    publisher_ (publisher),
    group_coherent_ (! (this->publisher_ == ::OpenDDS::DCPS::GUID_UNKNOWN))
  {}

  void operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample) {
    // Clear coherent_change_ flag; this makes
    // the data available for read/take operations.
    if (this->group_coherent_) {
      if (data_sample->coherent_change_
          && (this->publisher_ == data_sample->publisher_id_)) {
        data_sample->coherent_change_ = false;
      }
    }
    else {
      if (data_sample->coherent_change_ && (this->writer_ == data_sample->pub_)) {
        data_sample->coherent_change_ = false;
      }
    }
  }

private:

  OpenDDS::DCPS::PublicationId& writer_;
  OpenDDS::DCPS::RepoId& publisher_;
  bool group_coherent_;
};

} // namespace

#endif // OPENDDS_NO_OBJECT_MODEL_PROFILE

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
void
ReceivedDataStrategy::accept_coherent(PublicationId& writer,
                                      RepoId& publisher)
{
  CoherentFilter    filter = CoherentFilter(writer, publisher);
  AcceptCoherent operation = AcceptCoherent(writer, publisher);
  this->rcvd_samples_.apply_all(filter, operation);
}

void
ReceivedDataStrategy::reject_coherent(PublicationId& writer,
                                      RepoId& publisher)
{
  CoherentFilter filter = CoherentFilter(writer, publisher);
  this->rcvd_samples_.remove(filter, true);
}
#endif

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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

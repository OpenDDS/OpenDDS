/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Time_Helper.h"
#include "ReceivedDataElementList.h"
#include "ReceivedDataStrategy.h"
#include "GuidUtils.h"

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

namespace {

class CoherentFilter : public OpenDDS::DCPS::ReceivedDataFilter {
public:
  CoherentFilter(const OpenDDS::DCPS::PublicationId& writer,
                 const OpenDDS::DCPS::RepoId& publisher)
  : writer_(writer),
    publisher_(publisher),
    group_coherent_(publisher_ != ::OpenDDS::DCPS::GUID_UNKNOWN)
  {}

  bool operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample)
  {
    if (group_coherent_) {
      return data_sample->coherent_change_
             && (publisher_ == data_sample->publisher_id_);
    } else {
      return data_sample->coherent_change_
             && (writer_ == data_sample->pub_);
    }
  }

private:

  const OpenDDS::DCPS::PublicationId& writer_;
  const OpenDDS::DCPS::RepoId& publisher_;
  bool group_coherent_;
};

class AcceptCoherent : public OpenDDS::DCPS::ReceivedDataOperation {
public:
  AcceptCoherent(const OpenDDS::DCPS::PublicationId& writer,
                 const OpenDDS::DCPS::RepoId& publisher,
                 OpenDDS::DCPS::ReceivedDataElementList* rdel)
    : writer_(writer)
    , publisher_(publisher)
    , rdel_(rdel)
    , group_coherent_(publisher_ != ::OpenDDS::DCPS::GUID_UNKNOWN)
  {}

  void operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample)
  {
    // Clear coherent_change_ flag; this makes
    // the data available for read/take operations.
    if (group_coherent_) {
      if (data_sample->coherent_change_
          && (publisher_ == data_sample->publisher_id_)) {
        rdel_->accept_coherent_change(data_sample);
      }
    } else {
      if (data_sample->coherent_change_ && (writer_ == data_sample->pub_)) {
        rdel_->accept_coherent_change(data_sample);
      }
    }
  }

private:

  const OpenDDS::DCPS::PublicationId& writer_;
  const OpenDDS::DCPS::RepoId& publisher_;
  OpenDDS::DCPS::ReceivedDataElementList* rdel_;
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
ReceivedDataStrategy::accept_coherent(const PublicationId& writer,
                                      const RepoId& publisher)
{
  CoherentFilter filter(writer, publisher);
  AcceptCoherent operation(writer, publisher, &rcvd_samples_);
  rcvd_samples_.apply_all(filter, operation);
}

void
ReceivedDataStrategy::reject_coherent(const PublicationId& writer,
                                      const RepoId& publisher)
{
  CoherentFilter filter(writer, publisher);
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
  rcvd_samples_.add_by_timestamp(data_sample);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

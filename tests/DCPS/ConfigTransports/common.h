// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
 *  $Id$
 *
 */
// ============================================================================

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"

#include <vector>
#include <string>

#define MY_DOMAIN 411
#define MY_SAME_TOPIC  "foo"
#define MY_OTHER_TOPIC  "bar"

namespace OpenDDS
{
  namespace DCPS
  {
    class TransportClient;
  }
}

class Options;

//bool wait_publication_matched_status(const Options& opts, const DDS::DataReader_var& e);

bool wait_publication_matched_status(const Options& opts, const DDS::Entity_ptr e);

bool assert_publication_matched(const Options& opts, const DDS::DataWriterListener_var& dwl);

bool assert_subscription_matched(const Options& opts, const DDS::DataReaderListener_var& drl);

bool assert_supports_all(const Options& opts, const DDS::Entity_ptr e);

bool assert_supports_all(const Options& opts, const OpenDDS::DCPS::TransportClient* tc, const std::vector<std::string>& transporti);

bool assert_negotiated(const Options& options, const DDS::Entity_ptr e);

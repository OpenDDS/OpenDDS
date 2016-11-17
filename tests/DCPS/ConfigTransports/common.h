// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS
{
  namespace DCPS
  {
    class TransportClient;
  }
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

extern ACE_Thread_Mutex shutdown_lock;
extern bool shutdown_flag;

class Options;

bool wait_subscription_matched_status(const Options& opts, const DDS::DataReader_ptr r);
bool wait_publication_matched_status(const Options& opts, const DDS::DataWriter_ptr e);

bool assert_publication_matched(const Options& opts, const DDS::DataWriterListener_var& dwl);

bool assert_subscription_matched(const Options& opts, const DDS::DataReaderListener_var& drl);

bool assert_supported(const Options& opts, const DDS::Entity_ptr e);

bool assert_negotiated(const Options& opts, const DDS::Entity_ptr e);

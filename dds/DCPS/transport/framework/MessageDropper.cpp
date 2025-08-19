/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "MessageDropper.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef OPENDDS_TESTING_FEATURES
#include <cstdlib>
#ifdef _MSC_VER
#define OPENDDS_DRAND48() (rand()*(1./RAND_MAX))
#else
#define OPENDDS_DRAND48 drand48
#endif
#endif

void
MessageDropper::reload(RcHandle<ConfigStoreImpl> config_store,
                       const String& config_prefix)
{
  drop_messages_ = config_store->get_boolean((config_prefix + "_DROP_MESSAGES").c_str(), false);
  drop_messages_m_ = config_store->get_float64((config_prefix + "_DROP_MESSAGES_M").c_str(), 0);
  drop_messages_b_ = config_store->get_float64((config_prefix + "_DROP_MESSAGES_B").c_str(), 0);
}

bool
MessageDropper::should_drop(ssize_t length) const
{
#ifdef OPENDDS_TESTING_FEATURES
  return drop_messages_ && (OPENDDS_DRAND48() < (length * drop_messages_m_ + drop_messages_b_));
#else
  ACE_UNUSED_ARG(length);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: MessageDropper::should_drop: "
             "caller not conditioned on OPENDDS_TESTING_FEATURES\n"));
  return false;
#endif
}

bool
MessageDropper::should_drop(const iovec iov[],
                            int n,
                            ssize_t& length) const
{
  length = 0;
  for (int i = 0; i < n; ++i) {
    length += static_cast<ssize_t>(iov[i].iov_len);
  }
  return should_drop(length);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

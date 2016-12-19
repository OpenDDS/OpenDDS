/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATE_RECEIVER_T_H
#define UPDATE_RECEIVER_T_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Task.h"

#include <list>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {

struct SampleInfo;

} // namespace DDS

namespace OpenDDS {
namespace Federator {

template<class DataType>
class UpdateProcessor;

template<class DataType>
class  UpdateReceiver : public virtual ACE_Task_Base {
public:
  /// Construct with a processor reference.
  UpdateReceiver(UpdateProcessor<DataType>& processor);

  virtual ~UpdateReceiver();

  // ACE_Task_Base methods.

  virtual int open(void*);
  virtual int svc();
  virtual int close(u_long flags = 0);

  /**
   * @brief Sample enqueueing.
   *
   * @param sample - pointer to the received sample to be processed
   * @param info   - pointer to the info about the sample to be processed.
   *
   * NOTE: We take ownership of this data and delete it when we are
   *       done processing it.
   */
  void add(DataType* sample, DDS::SampleInfo* info);

  /// Synchronous termination.
  void stop();

private:
  /// The object that we delegate update processing to.
  UpdateProcessor<DataType>& processor_;

  /// Termination flag.
  bool stop_;

  /// Protect queue modifications.
  ACE_SYNCH_MUTEX lock_;

  /// Work to do indicator.
  ACE_Condition<ACE_SYNCH_MUTEX> workAvailable_;

  /// Contents of the queue.
  typedef std::pair<DataType*, DDS::SampleInfo* > DataInfo;

  /// Queue of publication data to process.
  std::list<DataInfo> queue_;
};

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "UpdateReceiver_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message ("UpdateReceiver_T.cpp template inst")
#pragma implementation ("UpdateReceiver_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif  /* UPDATE_RECEIVER_T_H */

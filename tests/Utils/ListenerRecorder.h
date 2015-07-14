/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TestUtils_ListenerRecorder_IMPL_H
#define TestUtils_ListenerRecorder_IMPL_H

#include <tests/Utils/DataReaderListenerImpl.h>
#include "ace/Thread_Mutex.h"

#include <vector>

namespace TestUtils {

template<typename Message, typename MessageDataReader>
class ListenerRecorder
  : public virtual TestUtils::DataReaderListenerImpl<Message, MessageDataReader>
{
public:
  typedef std::vector<Message> Messages;
  ListenerRecorder()
  { }

  Messages messages() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, Messages());
    return messages_;
  }
protected:
  virtual void on_sample(const Message& msg)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    messages_.push_back(msg);
    if (this->verbose()) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) ListenerRecorder::on_sample: Received message #%d\n",
                 messages_.size()));
    }
  }

  Messages messages_;
  mutable ACE_Thread_Mutex lock_;
};

}
#endif /* TestUtils_ListenerRecorder_IMPL_H */

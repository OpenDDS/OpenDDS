/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_TOPIC_H
#define OPENDDS_DCPS_INTERNAL_TOPIC_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "PoolAllocator.h"
#include "RcObject.h"
#include "InternalDataWriter.h"
#include "InternalDataReader.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class InternalTopic : public RcObject {
public:
  typedef RcHandle<InternalDataWriter<T> > InternalDataWriter_rch;
  typedef RcHandle<InternalDataReader<T> > InternalDataReader_rch;

  void connect(InternalDataWriter_rch writer)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    const std::pair<typename WriterSet::iterator, bool> p = writers_.insert(writer);

    if (p.second) {
      for (typename ReaderSet::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
        writer->add_reader(*pos);
      }
    }
  }

  void connect(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    const std::pair<typename ReaderSet::iterator, bool> p = readers_.insert(reader);

    if (p.second) {
      for (typename WriterSet::const_iterator pos = writers_.begin(), limit = writers_.end(); pos != limit; ++pos) {
        (*pos)->add_reader(reader);
      }
    }
  }

  void disconnect(InternalDataWriter_rch writer)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (writers_.erase(writer)) {
      for (typename ReaderSet::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
        writer->remove_reader(*pos);
      }
    }
  }

  void disconnect(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (readers_.erase(reader)) {
      for (typename WriterSet::const_iterator pos = writers_.begin(), limit = writers_.end(); pos != limit; ++pos) {
        (*pos)->remove_reader(reader);
      }
    }
  }

private:
  typedef OPENDDS_SET(InternalDataWriter_rch) WriterSet;
  WriterSet writers_;

  typedef OPENDDS_SET(InternalDataReader_rch) ReaderSet;
  ReaderSet readers_;

  mutable ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_INTERNAL_TOPIC_H

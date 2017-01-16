/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RCOBJECT_T_H
#define OPENDDS_RCOBJECT_T_H

#include "dds/DCPS/dcps_export.h"
#include "ace/Atomic_Op.h"
#include "ace/Malloc_Base.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL


namespace OpenDDS {
namespace DCPS {

/// Templated reference counting mix-in.
/// A non-DDS specific helper class.
/// The T type is an ace lock type
/// (eg, ACE_SYNCH_MUTEX, ACE_NULL_MUTEX, etc...)
template <typename T>
  class OpenDDS_Dcps_Export RcObject : public PoolAllocationBase {
public:

  virtual ~RcObject() {}

  virtual void _add_ref() {
    ++this->ref_count_;
  }

  virtual void _remove_ref() {
    long new_count = --this->ref_count_;

    if (new_count == 0) {
      // No need to protect the allocator with a lock since this
      // is the last reference to this object, and thus only one
      // thread will be doing this final _remove_ref().
      ACE_Allocator* allocator = this->allocator_;
      this->allocator_ = 0;

      if (allocator) {
        this->~RcObject();
        allocator->free(this);
      } else {
        delete this;
      }
    }
  }

  /// This accessor is purely for debugging purposes
  long ref_count() const {
    return this->ref_count_.value();
  }

protected:

  RcObject(ACE_Allocator* allocator = 0)
    : ref_count_(1), allocator_(allocator)
  {}

private:

  ACE_Atomic_Op<T, long> ref_count_;
  ACE_Allocator*         allocator_;

  // Turning these off.  I don't think they should be used for
  // objects that would be reference counted.  Maybe a copy_from()
  // method instead (if needed).
  RcObject(const RcObject&);
  RcObject& operator=(const RcObject&);
};


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /*OPENDDS_RCOBJECT_T_H */

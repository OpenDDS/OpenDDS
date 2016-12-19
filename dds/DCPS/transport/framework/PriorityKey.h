/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PRIORITY_KEY_H
#define PRIORITY_KEY_H

#include "dds/DCPS/dcps_export.h"
#include "TransportDefs.h"
#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class PriorityKey
 *
 * @brief Encapsulate a priority value and internet address as a key.
 *
 * This class is encapsulates a priority value and an
 * internet address value for use as a key in either an STL container
 * or an ACE hash container.  The '<' operator is used by the STL
 * containers and the '==' operator and hash() method are used by the
 * ACE hash map container(s).  The ACE hash map container(s) also
 * require the use of a default constructor as well.
 *
 * To use keys of this type as an STL container key, simply include
 * this type as the key template parameter.  An example usage is:
 *
 *   typedef std::map<PriorityKey, ValueType> PriorityMap;
 *
 * To use this type as an ACE hash container key, use the function
 * object templates ACE_Hash and ACE_Equal_To for the HASH_KEY and
 * COMPARE_KEYS template parameters.  An example usage is:
 *
 *   typedef ACE_Hash_Map_Manager_Ex<
 *             PriorityKey,
 *             ValueType,
 *             ACE_Hash<PriorityKey>,
 *             ACE_Equal_To<PriorityKey>,
 *             SynchType
 *           > PriorityHashMap;
 *
 *  Default copy constructor and assigment are sufficient.  Readonly
 *  and read/write accessors for member data are provided.
 */
class OpenDDS_Dcps_Export PriorityKey {
public:
  // Default constructor.
  PriorityKey();

  // Construct with values.
  PriorityKey(Priority priority, ACE_INET_Addr address, bool is_loopback, bool active);

  // Ordering for STL containers.
  bool operator<(const PriorityKey& rhs) const;

  // Ordering (location identification) for ACE_Hash_Map_Manager_Ex.
  bool operator==(const PriorityKey& rhs) const;

  // Identity for ACE_Hash_Map_Manager_Ex.
  unsigned long hash() const;

  // Access priority value.
  Priority& priority();
  Priority  priority() const;

  // Access address value.
  ACE_INET_Addr& address();
  ACE_INET_Addr  address() const;

  bool& is_loopback();
  bool  is_loopback() const;

  bool& is_active();
  bool  is_active() const;

private:
  // Priority value of key.
  Priority priority_;

  // Address value of key.
  ACE_INET_Addr address_;

  bool is_loopback_;
  bool is_active_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "PriorityKey.inl"
#endif /* __ACE_INLINE__ */

#endif  /* PRIORITY_KEY_H */

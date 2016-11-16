/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_UTIL_H
#define OPENDDS_DCPS_UTIL_H

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// bind reproduces the ACE_Hash_Map_Manager_Ex's bind behavior
template <typename Container, typename FirstType, typename SecondType>
int bind(
  Container& c,
  const FirstType& first,
  const SecondType& second)
{
  if (c.find(first) == c.end()) {
    typedef typename Container::value_type container_value_type;

    if (c.insert(container_value_type(first, second)).second) {
      return 0;
    }

    return -1;
  }

  return 1;
}

// unbind reproduces the ACE_Hash_Map_Manager_Ex's unbind behavior
template <typename Container>
int unbind(
  Container& c,
  const typename Container::key_type& k,
  typename Container::mapped_type& v)
{
  typename Container::const_iterator iter = c.find(k);

  if (iter != c.end()) {
    v = iter->second;

    if (c.erase(k) == 1) {
      return 0;
    }

    return -1;
  }

  return -1;
}

// unbind reproduces the ACE_Hash_Map_Manager_Ex's unbind behavior
template <typename Container>
int unbind(
  Container& c,
  const typename Container::key_type& k)
{
  typename Container::mapped_type v;
  return unbind(c, k, v);
}

template <typename Container, typename Key>
int find(
  Container& c,
  const Key& key,
  typename Container::mapped_type*& value)
{
  typename Container::iterator iter =
    c.find(key);

  if (iter == c.end()) {
    return -1;
  }

  value = &iter->second;
  return 0;
}

template <typename Container, typename Key>
int find(
  const Container& c,
  const Key& key,
  typename Container::mapped_type& value)
{
  typename Container::const_iterator iter =
    c.find(key);

  if (iter == c.end()) {
    return -1;
  }

  value = iter->second;
  return 0;
}

template <typename Container, typename ValueType>
int insert(
  Container& c,
  const ValueType& v)
{
  if (c.find(v) == c.end()) {
    if (c.insert(v).second) {
      return 0;
    }

    return -1;
  }

  return 1;
}

template <typename Container, typename ValueType>
int remove(
  Container& c,
  const ValueType& v)
{
  if (c.find(v) != c.end()) {
    if (c.erase(v) == 1) {
      return 0;
    }

    return -1;
  }

  return -1;
}

/// std::vector-style push_back() for CORBA Sequences
template <typename Seq>
void push_back(Seq& seq, const typename Seq::value_type& val)
{
  const CORBA::ULong len = seq.length();
  seq.length(len + 1);
  seq[len] = val;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_UTIL_H */

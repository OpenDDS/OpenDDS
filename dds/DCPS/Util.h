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

// Constructs a sorted intersect of the given two sorted ranges [a,aEnd) and [b,bEnd).
// (for pre-c++17 code for the similar effect of std::set_intersection in c++17)
template <typename InputIteratorA, typename InputIteratorB, typename OutputIterator>
OutputIterator intersect_sorted_ranges(InputIteratorA a, InputIteratorA aEnd,
                                       InputIteratorB b, InputIteratorB bEnd,
                                       OutputIterator intersect)
{
  while (a != aEnd && b != bEnd) {
    if (*a < *b) { ++a; }
    else if (*b < *a) { ++b; }
    else { *intersect++ = *a++; ++b; }
  }
  return intersect;
}

// Constructs a sorted intersect of the given two sorted ranges [a,aEnd) and [b,bEnd).
// (for pre-c++17 code for the similar effect of std::set_intersection in c++17)
template <typename InputIteratorA, typename InputIteratorB, typename OutputIterator, typename LessThan>
OutputIterator intersect_sorted_ranges(InputIteratorA a, InputIteratorA aEnd,
                                       InputIteratorB b, InputIteratorB bEnd,
                                       OutputIterator intersect, LessThan lessThan)
{
  while (a != aEnd && b != bEnd) {
    if (lessThan(*a, *b)) { ++a; }
    else if (lessThan(*b, *a)) { ++b; }
    else { *intersect++ = *a++; ++b; }
  }
  return intersect;
}

// Keeps the intersection of the two sorted collections in the first one,
// and returns whether an intersection exists between the two colloctions.
// Note:
// The generic scope of the first template parameter is narrowed down to std::set<T> because
// the pre-c++11 erase() may not work here with some collections (e.g. a sorted std::vector).
// The since-c++11 erase() works properly with both an std::set and a sorted std::vector.
template <typename SetA, typename SortedB, typename LessThan>
bool set_intersect(SetA& sA, const SortedB& sB, LessThan lessThan)
{
  typename SetA::iterator a = sA.begin();
  typename SortedB::const_iterator b = sB.begin();
  while (a != sA.end()) {
    if (b != sB.end()) {
      if (lessThan(*a, *b)) {
        sA.erase(a++);
      } else {
        if (!lessThan(*b, *a)) { ++a; }
        ++b;
      }
    } else {
      sA.erase(a, sA.end());
      break;
    }
  }
  return !sA.empty();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_UTIL_H */

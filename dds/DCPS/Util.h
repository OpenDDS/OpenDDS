#ifndef TAO_DCPS_UTIL_H
#define TAO_DCPS_UTIL_H

namespace TAO
{
  namespace DCPS
  {
    // bind reproduces the ACE_Hash_Map_Manager_Ex's bind behavior
    template <typename Container>
    int bind(
      Container& c,
      const typename Container::value_type& v
      )
    {
      if (c.find(v.first) == c.end())
      {
        if (c.insert(v).second)
        {
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
      typename Container::mapped_type& v
      )
    {
      typename Container::const_iterator iter = c.find(k);
      if (iter != c.end())
      {
        v = iter->second;
        if (c.erase(k) == 1)
        {
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
      const typename Container::key_type& k
      )
    {
      typename Container::mapped_type v;
      return unbind(c, k, v);
    }

    template <typename Container, typename Key>
      int find(
      Container& c,
      const Key& key,
      typename Container::mapped_type*& value
      )
    {
      typename Container::iterator iter =
        c.find(key);
      if (iter == c.end())
      {
        return -1;
      }
      value = &iter->second;
      return 0;
    }

    template <typename Container, typename Key>
      int find(
      Container& c,
      const Key& key,
      typename Container::mapped_type& value
      )
    {
      typename Container::iterator iter =
        c.find(key);
      if (iter == c.end())
      {
        return -1;
      }
      value = iter->second;
      return 0;
    }
  }
}

#endif /* TAO_DCPS_UTIL_H */

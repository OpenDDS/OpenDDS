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
  }
}

#endif /* TAO_DCPS_UTIL_H */

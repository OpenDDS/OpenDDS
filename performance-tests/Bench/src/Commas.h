// -*- C++ -*-
//
#ifndef COMMAS_H
#define COMMAS_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <iosfwd>

class Commas {
  public:
    Commas( long value);
    std::ostream& operator()( std::ostream& str) const;

  private:
    long value_;
};

inline
Commas::Commas( long value)
 : value_( value)
{
}

inline
std::ostream& operator<<( std::ostream& str, const Commas& value)
{
  return value( str);
}

#endif // COMMAS_H


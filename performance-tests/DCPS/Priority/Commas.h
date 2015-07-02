// -*- C++ -*-
//
#ifndef COMMAS_H
#define COMMAS_H

#include <iosfwd>

class Commas {
  public:
    Commas( long value) : value_( value) { }
    std::ostream& operator()( std::ostream& str) const;

  private:
    long value_;
};

std::ostream& operator<<( std::ostream& str, const Commas& value);

#endif // COMMAS_H


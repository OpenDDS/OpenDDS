// -*- C++ -*-
//

#include "Commas.h"

#include <iostream>
#include <iomanip>

std::ostream&
Commas::operator()( std::ostream& str) const
{
  short triples[ 8]; // Large enough to hold 64 bit values.
  short index = 0;
  for( long value = this->value_; value != 0; value /= 1000) {
    triples[ index++] = value % 1000;
  }

  char original = str.fill(' ');
  while( index > 0) {
    str << std::dec << std::setw(3) << triples[ --index];
    str.fill('0');
    if( index != 0) str << ",";
  }
  str.fill( original);

  return str;
}


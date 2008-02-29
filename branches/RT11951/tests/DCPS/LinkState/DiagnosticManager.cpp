
#include "DiagnosticManager.h"

#include <iostream>
#include <iomanip>

namespace { // Anonymous namespace for file scope.
  /// Desired column width of output.
  enum { COLS = 80 };

} // End of anonymous namespace.


std::ostream& operator<<( std::ostream& str, DiagnosticManager* manager)
{
   manager->dump( str);
   return str;
}

std::ostream& operator<<( std::ostream& str, DiagnosticManager& manager)
{
   return str << &manager;
}

DiagnosticManager::DiagnosticManager()
{
}

DiagnosticManager::~DiagnosticManager()
{
}

void
DiagnosticManager::dump( std::ostream& str) const
{
  str << std::endl << "TOPOLOGY:" << std::endl;
  this->dumpAdjacencies( str);

  str << std::endl << "MINIMUM SPANNING TREE (MST):" << std::endl;
  this->dumpMst( str);

  str << std::endl;
}

void
DiagnosticManager::dumpAdjacencies( std::ostream& str) const
{
  int col = 0;
  char prev = str.fill('0');
  for( AdjacencyRow::const_iterator currentRow = this->adjacencies_.begin();
       currentRow != this->adjacencies_.end();
       ++currentRow) {
    for( AdjacencyColumn::const_iterator currentColumn = currentRow->second.begin();
         currentColumn != currentRow->second.end();
         ++currentColumn) {
      str << "(0x" << std::hex << std::setw(8) << currentRow->first;
      str << ",0x" << std::hex << std::setw(8) << currentColumn->first;
      str << ",0x" << std::hex << std::setw(8) << currentColumn->second.first;
      str << ",0x" << std::hex << std::setw(8) << currentColumn->second.second;
      str << ") ";
      col += 46;
      if( col > COLS) {
        str << std::endl;
        col = 0;
      }
    }
  }
  str.fill(prev);
}

void
DiagnosticManager::dumpMst( std::ostream& str) const
{
  int col = 0;
  char prev = str.fill('0');
  for( LinkSet::const_iterator location = this->mst_.begin();
       location != this->mst_.end();
       ++location) {
    str << "(0x" << std::hex << std::setw(8) << location->first;
    str << ",0x" << std::hex << std::setw(8) << location->second;
    str << ") ";
    col += 24;
    if( col > COLS) {
      str << std::endl;
      col = 0;
    }
  }
  str.fill(prev);
}


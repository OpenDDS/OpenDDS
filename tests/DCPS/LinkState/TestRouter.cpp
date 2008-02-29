
#include "TestRouter.h"
#include "TestException.h"

#include <iostream>
#include <iomanip>

namespace { // Anonymous namespace for file scope.

OpenDDS::Federator::LinkState testData[] = {
  { 1, 2, OpenDDS::Federator::LINK_ON, 1 },
  { 1, 3, OpenDDS::Federator::LINK_ON, 2 },
  { 1, 4, OpenDDS::Federator::LINK_ON, 3 },
  { 2, 1, OpenDDS::Federator::LINK_ON, 1 },
  { 2, 3, OpenDDS::Federator::LINK_ON, 2 },
  { 2, 4, OpenDDS::Federator::LINK_ON, 3 },
  { 3, 1, OpenDDS::Federator::LINK_ON, 1 },
  { 3, 2, OpenDDS::Federator::LINK_ON, 2 },
  { 3, 4, OpenDDS::Federator::LINK_ON, 3 },
  { 4, 1, OpenDDS::Federator::LINK_ON, 1 },
  { 4, 2, OpenDDS::Federator::LINK_ON, 2 },
  { 4, 3, OpenDDS::Federator::LINK_ON, 3 },

  { 3, 1, OpenDDS::Federator::LINK_OFF, 4 },
  { 1, 3, OpenDDS::Federator::LINK_OFF, 4 },

  { 3, 1, OpenDDS::Federator::LINK_ON, 3 },
  { 1, 3, OpenDDS::Federator::LINK_ON, 3 },

  { 3, 1, OpenDDS::Federator::LINK_ON, 4 },
  { 1, 3, OpenDDS::Federator::LINK_ON, 4 },

  { 3, 1, OpenDDS::Federator::LINK_ON, 5 },
  { 1, 3, OpenDDS::Federator::LINK_ON, 5 },

  { 1, 3, OpenDDS::Federator::LINK_OFF, 6 },
  { 1, 4, OpenDDS::Federator::LINK_OFF, 4 },
  { 2, 4, OpenDDS::Federator::LINK_OFF, 4 }
};

} // End of anonymous namespace

/**
 * @brief Construct a test system from the command line.
 */
TestRouter::TestRouter( int argc, char** argv, char** envp)
 : config_( argc, argv, envp)
{
  this->manager_.repoId() = 1;
}

TestRouter::~TestRouter()
{
}

void
TestRouter::run()
{
  std::cout << this->manager_;

  OpenDDS::Federator::LinkStateManager::LinkList added;
  OpenDDS::Federator::LinkStateManager::LinkList removed;
  for( unsigned int index = 0; index < (sizeof(testData)/sizeof(testData[0])); ++index) {
    char prev = std::cout.fill('0');
    std::cout << std::endl << "----------------------------------------" << std::endl;
    std::cout << "Updating with new LinkState "
              << "(0x" << std::hex << std::setw(8) << testData[index].source
              << ",0x" << std::hex << std::setw(8) << testData[index].destination
              << "," << std::dec << testData[index].cost
              << ",0x" << std::hex << std::setw(4) << testData[index].packet
              << ")" << std::endl;
    std::cout.fill(prev);

    bool result = this->manager_.update( testData[ index], removed, added);
    if( result == true) {
      std::cout << "Update WAS processed." << std::endl;
    } else {
      std::cout << "Update was NOT processed." << std::endl;
    }
    std::cout << this->manager_;

    std::cout << std::endl << std::dec << removed.size() << " LINKS REMOVED FROM MST:" << std::endl;
    for( unsigned int index = 0; index < removed.size(); ++index) {
      int col = 0;
      char prev = std::cout.fill('0');
      for( OpenDDS::Federator::LinkStateManager::LinkList::const_iterator location = removed.begin();
           location != removed.end();
           ++location) {
        std::cout << "(0x" << std::hex << std::setw(8) << location->first;
        std::cout << ",0x" << std::hex << std::setw(8) << location->second;
        std::cout << ") ";
        col += 24;
        if( col > 80) {
          std::cout << std::endl;
          col = 0;
        }
      }
      std::cout.fill(prev);
    }

    std::cout << std::endl << std::dec << added.size() << " LINKS ADDED TO MST:" << std::endl;
    for( unsigned int index = 0; index < added.size(); ++index) {
      int col = 0;
      char prev = std::cout.fill('0');
      for( OpenDDS::Federator::LinkStateManager::LinkList::const_iterator location = added.begin();
           location != added.end();
           ++location) {
        std::cout << "(0x" << std::hex << std::setw(8) << location->first;
        std::cout << ",0x" << std::hex << std::setw(8) << location->second;
        std::cout << ") ";
        col += 24;
        if( col > 80) {
          std::cout << std::endl;
          col = 0;
        }
      }
      std::cout.fill(prev);
    }

  }
  std::cout << std::endl;
}


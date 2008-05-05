#ifndef DIAGNOSTICMANAGER_H
#define DIAGNOSTICMANAGER_H

#include "dds/InfoRepo/LinkStateManager.h"

#include <iosfwd>

class DiagnosticManager : public OpenDDS::Federator::LinkStateManager {
  public:
    /// Default constructor.
    DiagnosticManager();

    /// Virtual destructor.
    virtual ~DiagnosticManager();

    /// Produce diagnostic output on a stream.
    void dump( std::ostream& str) const;

  protected:
    /// Dump the current Adjacency matrix (topology).
    void dumpAdjacencies( std::ostream& str) const;

    /// Dump the current Minimal Spanning Tree (MST).
    void dumpMst( std::ostream& str) const;

};

std::ostream& operator<<( std::ostream& str, DiagnosticManager* manager);

std::ostream& operator<<( std::ostream& str, DiagnosticManager& manager);

#endif // DIAGNOSTICMANAGER_H


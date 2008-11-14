// -*- C++ -*-
//
// $Id$
#ifndef OPTIONS_H
#define OPTIONS_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <iosfwd>

class Options  {
  public:
    /// Command line arguments.
    static const char* TRANSPORT_TYPE_ARGUMENT;   // Transport type
    static const char* PUBLISHER_ID_ARGUMENT;     // Publisher Id

    /// Types of transport implementations supported.
    enum TransportType {
      NONE,     // Unsupported
      TCP,      // SimpleTcp
      UDP,      // SimpleUnreliableDgram
      MC,       // SimpleUnreliableMcast
      RMC       // ReliableMulticast
    };
    friend std::ostream& operator<<( std::ostream& str, TransportType value);

    /// Default constructor.
    Options( int argc, char** argv, char** envp = 0);

    /// Virtual destructor.
    virtual ~Options();

    /// Transport Type value.
    protected: TransportType& transportType();
    public:    TransportType  transportType() const;

    /// Publisher id value.
    protected: long& publisherId();
    public:    long  publisherId() const;

  private:
    /// Transport Type value.
    TransportType transportType_;

    /// Publisher Id value.
    long publisherId_;
};

#if defined (__ACE_INLINE__)
# include "Options.inl"
#endif  /* __ACE_INLINE__ */

#endif // OPTIONS_H


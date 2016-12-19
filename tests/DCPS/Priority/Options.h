// -*- C++ -*-
//
#ifndef OPTIONS_H
#define OPTIONS_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <iosfwd>
#include <string>

namespace Test {

class Options  {
  public:
    /// Command line arguments.
    static const ACE_TCHAR* TRANSPORT_TYPE_ARGUMENT;   // Transport type
    static const ACE_TCHAR* PUBLISHER_ID_ARGUMENT;     // Publisher Id
    static const ACE_TCHAR* PRIORITY_ARGUMENT;         // Priority
    static const ACE_TCHAR* VERBOSE_ARGUMENT;          // Verbosity
    static const ACE_TCHAR* MULTI_ARGUMENT;            // Multiple Instances

    /// Types of transport implementations supported.
    enum TransportType {
      TRANSPORT_NONE,     // Unsupported (NONE is a macro on VxWorks)
      TCP,      // Tcp
      UDP,      // udp
      MC        // multicast
    };
    friend std::ostream& operator<<( std::ostream& str, TransportType value);

    Options( int argc, ACE_TCHAR** argv, char** envp = 0);

    virtual ~Options();

    /// Test verbosity.
    protected: bool& verbose();
    public:    bool  verbose() const;

    /// Test domain.
    protected: unsigned long& domain();
    public:    unsigned long  domain() const;

    /// Writer priority.
    protected: unsigned long& priority();
    public:    unsigned long  priority() const;

    /// Transport Type value.
    protected: TransportType& transportType();
    public:    TransportType  transportType() const;

    /// Transport Key value, translated from the type.
    protected: unsigned int& transportKey();
    public:    unsigned int  transportKey() const;

    /// Publisher id value.
    protected: long& publisherId();
    public:    long  publisherId() const;

    /// Test topic name.
    protected: std::string& topicName();
    public:    std::string topicName() const;

    /// Test multiple instances.
    protected: bool& multipleInstances();
    public:    bool  multipleInstances() const;

  private:
    /// Test verbosity.
    bool verbose_;

    /// Test domain.
    unsigned long domain_;

    /// Writer priority.
    unsigned long priority_;

    /// Transport Type value.
    TransportType transportType_;

    /// Transport Key value.
    unsigned int transportKey_;

    /// Publisher Id value.
    long publisherId_;

    /// Topic name.
    std::string topicName_;

    /// Test with multiple instances.
    bool multipleInstances_;
};

} // End of namespace Test

#if defined (__ACE_INLINE__)
# include "Options.inl"
#endif  /* __ACE_INLINE__ */

#endif // OPTIONS_H


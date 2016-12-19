// -*- C++ -*-
//
#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

namespace Test {

/**
 * @class Options
 *
 * @brief manage execution options
 *
 * This class extracts option information from the command line and makes
 * it available to the process.
 *
 * Options extracted by this class are:
 *
 *   -v
 *      Be verbose when executing.
 */
class Options  {
  public:
    /// Default constructor.
    Options( int argc, ACE_TCHAR** argv, char** envp = 0);

    virtual ~Options();

    /// Check validity of command line.
    operator bool() const;

    ///{ @name Test verbosity.
    protected: bool& verbose();
    public:    bool  verbose() const;
    ///}

    ///{ @name Test publisher.
    protected: bool& publisher();
    public:    bool  publisher() const;
    ///}

    ///{ @name Test domain.
    protected: unsigned long& domain();
    public:    unsigned long  domain() const;
    ///}

    ///{ @name Test topic name.
    protected: std::string& topicName();
    public:    std::string  topicName() const;
    ///}

    ///{ @name Number of publications.
    protected: long& publications();
    public:    long  publications() const;
    ///}

    ///{ @name Transport key.
    protected: unsigned int& transportKey();
    public:    unsigned int  transportKey() const;
    ///}

  private:
    /// Validity of command line.
    bool valid_;

    /// Test verbosity.
    bool verbose_;

    /// Test Publisher implementation.
    bool publisher_;

    /// Test domain.
    unsigned long domain_;

    /// Topic name for test.
    std::string topicName_;

    /// Number of publications.
    long publications_;

    /// Index into transport table.
    unsigned int transportKey_;
};

} // End of namespace Test

#if defined (__ACE_INLINE__)
# include "Options.inl"
#endif  /* __ACE_INLINE__ */

#endif // OPTIONS_H


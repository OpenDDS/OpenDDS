// -*- C++ -*-
//
#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#include "Process.h"

#include <dds/DdsDcpsSubscriptionExtC.h>

#include <string>
#include <map>
#include <iosfwd>

namespace Test {

struct SubscriptionProfile;
class  DataReaderListener;
class  Publication;

class Subscription {
  public:
    /// Construct with a profile.
    Subscription(
      const char* name,
      SubscriptionProfile* profile,
      bool verbose = false
    );

    virtual ~Subscription();

    /// Resource management.
    void enable(
      DDS::DomainParticipant_ptr participant,
      DDS::Topic_ptr             topic
    );

    /// @name State access
    /// @{

    /// Number of active associations.
    int associations() const;

    /// Flag indicating that publications are still associated.
    bool active() const;

    /// Number of received messages.
    int total_messages() const;

    /// Number of valid messages.
    int valid_messages() const;

    /// Number of samples received during the test from each writer.
    const std::map< long, long>& counts() const;

    /// Number of payload bytes received during the test from each writer.
    const std::map< long, long>& bytes() const;

    /// Priority of  writers.
    const std::map< long, long>& priorities() const;

    /// @}

    /// @name DataReader interfaces.
    /// @{
    DDS::StatusCondition_ptr get_statuscondition();
    /// @}

    /// Establish a forwarding destination.
    void set_destination( Publication* publication);

    /// Format and dump raw data to a stream.
    std::ostream& rawData( std::ostream& str) const;

    /// Format and dump summary data to a stream.
    std::ostream& summaryData( std::ostream& str) const;

  private:
    /// Name of this publication.
    std::string name_;

    /// Profile configuring this publication.
    SubscriptionProfile* profile_;

    /// Verbosity indication.
    bool verbose_;

    /// Resource status.
    bool enabled_;

    /// The reader for the subscription.
    OpenDDS::DCPS::DataReaderEx_var reader_;

    /// Reader listener.
    DataReaderListener* listener_;

    /// Reader listener lifetime management.
    OpenDDS::DCPS::LocalObject_var safe_listener_;
};

} // End of namespace Test

/// Stream out statistics values.
inline
std::ostream& operator<<( std::ostream& str, const Test::Subscription& value)
{
  return value.summaryData( str);
}

/// Convenience insertion operator.
std::ostream&
operator<<( std::ostream& str, const OpenDDS::DCPS::LatencyStatisticsSeq& statistics);

#endif /* SUBSCRIPTION_H */


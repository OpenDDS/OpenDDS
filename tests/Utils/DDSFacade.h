/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TestUtils_Export.h"

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include <model/Sync.h>

namespace TestUtils {

class DDSApp;

/// Facade to represent a Topic and all the data readers and writers on the
/// Topic.  The class will create or use default parameters when they are
/// not provided.  Since a Topic only has one participant, all data readers
/// and data writers are created on the same participant.
///
/// publishers: When a reader is created a publisher can either be
/// explicitly provided (from a previous call to DDSApp.publisher) or is
/// the default publisher.  The default publisher is set to the first
/// created publisher (either set to the first publisher explicitly passed
/// to writer(...) or to the implicitly created publisher from calling
/// writer() with no publisher).
///
///
/// subscribers: When a reader is created a subscriber can either be
/// explicitly provided (from a previous call to DDSApp.subscriber) or is
/// the default subscriber.  The default subscriber is set to the first
/// created subscriber (either set to the first subscriber explicitly passed
/// to reader(...) or to the implicitly created subscriber from calling
/// reader() with no subscriber).
template<typename WriterOrReaderImpl>
// TODO: change codegen and use Message
class DDSFacade
{
public:
  friend class DDSApp;
  typedef typename WriterOrReaderImpl::typesupportimpl_type  typesupportimpl_type;
  typedef typename typesupportimpl_type::datawriter_type     datawriter_type;
  typedef typename typesupportimpl_type::datawriter_var      datawriter_var;

  datawriter_var writer(DDS::Publisher_var publisher = DDS::Publisher_var())
  {
    determine_publisher(publisher);
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic_.in(),
                                   DATAWRITER_QOS_DEFAULT,
                                   DDS::DataWriterListener::_nil(),
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    return datawriter_var(datawriter_type::_narrow(writer.in()));
  }

  DDS::DataReader_var reader(DDS::DataReaderListener_var listener,
                             DDS::Subscriber_var subscriber = DDS::Subscriber_var())
  {
    determine_subscriber(subscriber);
    return subscriber->create_datareader(topic_.in(),
                                         DATAREADER_QOS_DEFAULT,
                                         listener.in(),
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  }

private:
  // only called by DDSApp
  DDSFacade(DDS::DomainParticipant_var participant,
            DDS::Topic_var topic)
  : participant_(participant)
  , topic_(topic)
  { }

  // TODO: REMOVE
  DDSFacade() {}

  void determine_publisher(DDS::Publisher_var& publisher)
  {
    if (publisher.in())
      return;

    if (!default_pub_.in()) {
      default_pub_ =
        participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    }

    publisher = default_pub_;
  }

  void determine_subscriber(DDS::Subscriber_var& subscriber)
  {
    if (subscriber.in())
      return;

    if (!default_sub_.in()) {
      default_sub_ =
        participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                        DDS::SubscriberListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    }

    subscriber = default_sub_;
  }

  DDS::DomainParticipant_var participant_;
  DDS::Topic_var             topic_;

  DDS::Publisher_var         default_pub_;
  DDS::DataWriter_var        default_dw_;

  DDS::Subscriber_var        default_sub_;
  DDS::DataReader_var        default_dr_;
};

} // End namespaces

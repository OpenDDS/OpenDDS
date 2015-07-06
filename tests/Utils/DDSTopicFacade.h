/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TestUtils_DDSTopicFacade_H
#define TestUtils_DDSTopicFacade_H

#include "TestUtils_Export.h"

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/TypeSupportImpl.h"

#include "dds/DCPS/Marked_Default_Qos.h"

#include <iostream>
#include <map>

namespace TestUtils {

class DDSApp;

template<typename Qos>
struct QosNoOp
{
  void operator()(Qos& )
  {
  }
};

/// Facade to represent a Topic and all the data readers and writers on the
/// Topic.  The class will create or use default parameters when they are
/// not provided.  Since a Topic only has one participant, all data readers
/// and data writers are created on the same participant.
///
/// publishers: When a writer is created a publisher can either be
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
template<typename MessageType>
// TODO: change codegen and use Message
class DDSTopicFacade
{
public:
  friend class DDSApp;
  typedef OpenDDS::DCPS::DDSTraits<MessageType> TraitsType;
  typedef typename TraitsType::DataWriterType         DataWriterType;
  typedef typename TraitsType::DataWriterType::_var_type     DataWriterVarType;
  typedef std::map<DDS::DataWriter_ptr, DDS::Publisher_var>  DataWriters;
  typedef std::map<DDS::DataReader_ptr, DDS::Subscriber_var> DataReaders;

  DataWriterVarType writer(DDS::Publisher_var          publisher = DDS::Publisher_var(),
                        DDS::DataWriterListener_var a_listener =
                          DDS::DataWriterListener::_nil(),
                        DDS::StatusMask             mask =
                          OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    return writer(publisher, QosNoOp<DDS::DataWriterQos>(), a_listener, mask);
  }

  template<typename QosFunc>
  DataWriterVarType writer(QosFunc                     qos_func)
  {
    DDS::Publisher_var publisher;
    determine_publisher(publisher);
    return writer(publisher,
                  qos_func,
                  DDS::DataWriterListener::_nil(),
                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  }

  template<typename QosFunc>
  DataWriterVarType writer(DDS::Publisher_var          publisher,
                        QosFunc                     qos_func,
                        DDS::DataWriterListener_var a_listener =
                          DDS::DataWriterListener::_nil(),
                        DDS::StatusMask             mask =
                          OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    determine_publisher(publisher);
    DDS::DataWriterQos qos;
    publisher->get_default_datawriter_qos(qos);
    qos_func(qos);
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic_.in(),
                                   qos,
                                   a_listener.in(),
                                   mask);
    return DataWriterVarType(DataWriterType::_narrow(writer.in()));
  }

  DDS::DataReader_var reader(DDS::DataReaderListener_var listener,
                             DDS::Subscriber_var         subscriber =
                               DDS::Subscriber_var(),
                             DDS::StatusMask             mask =
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    return reader(listener,
                  QosNoOp<DDS::DataReaderQos>(),
                  subscriber,
                  mask);
  }

  template<typename QosFunc>
  DDS::DataReader_var reader(DDS::DataReaderListener_var listener,
                             QosFunc                     qos_func,
                             DDS::Subscriber_var         subscriber =
                               DDS::Subscriber_var(),
                             DDS::StatusMask             mask =
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK)
  {
    determine_subscriber(subscriber);
    DDS::DataReaderQos qos;
    subscriber->get_default_datareader_qos(qos);
    qos_func(qos);
    return subscriber->create_datareader(topic_.in(),
                                         qos,
                                         listener.in(),
                                         mask);
  }

  void remove(DataWriterVarType& writer)
  {
    if (remove(writer.in(), "DataWriter", writers_))
      writer = 0;
  }

  void remove(DDS::DataWriter_var& writer)
  {
    if (remove(writer.in(), "DataWriter", writers_))
      writer = 0;
  }

  void remove(DDS::DataReader_var& reader)
  {
    if (remove(reader.in(), "DataReader", readers_))
      reader = 0;
  }

private:
  // only called by DDSApp
  DDSTopicFacade(DDS::DomainParticipant_var participant,
            DDS::Topic_var topic)
  : participant_(participant)
  , topic_(topic)
  { }

  // TODO: REMOVE
  DDSTopicFacade() {}

  bool remove_from_parent(const DDS::Publisher_var& pub, DDS::DataWriter_ptr datawriter)
  {
    DDS::ReturnCode_t ret = pub->delete_datawriter (datawriter);
    return ret == DDS::RETCODE_OK;
  }

  bool remove_from_parent(const DDS::Subscriber_var& sub, DDS::DataReader_ptr datareader)
  {
    DDS::ReturnCode_t ret = sub->delete_datareader (datareader);
    return ret == DDS::RETCODE_OK;
  }

  template<typename DDS_Ptr, typename Map>
  bool remove(const DDS_Ptr dds_ptr, const char* ptr_desc, Map& map)
  {
    typename Map::iterator iter = map.find(dds_ptr);
    if (iter == map.end()) {
      std::cerr << "ERROR: could not find " << ptr_desc
        << " to delete" << std::endl;
      return false;
    }

    if (!remove_from_parent(iter->second, iter->first))
      return false;

    map.erase(iter);
    return true;
  }

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

  DataWriters                writers_;
  DataReaders                readers_;
};

} // End namespaces

#endif /* TestUtils_DDSTopicFacade_H */

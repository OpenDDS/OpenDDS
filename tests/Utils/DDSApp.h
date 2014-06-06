/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TestUtils_Export.h"

#include "tests/Utils/DDSFacade.h"

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"

#include <model/Sync.h>
#include <map>
#include <string>

// Boilerplate code pulled out of mains of publisher and subscriber
// in order to simplify this example.
namespace TestUtils {

class TestUtils_Export DDSApp
{
public:
  typedef std::map<DDS::DomainParticipant_ptr, DDS::DomainParticipant_var> Participants;

  DDSApp(int argc, ACE_TCHAR* argv[]);
  DDSApp(int argc, ACE_TCHAR* argv[], ::DDS::DomainId_t default_domain_id);
  ~DDSApp();

  DDS::DomainParticipant_var participant();
  DDS::DomainParticipant_var participant(::DDS::DomainId_t domain_id);

  DDS::Publisher_var  publisher(
    DDS::DomainParticipant_var participant = DDS::DomainParticipant_var());
  DDS::Subscriber_var subscriber(
    DDS::DomainParticipant_var participant = DDS::DomainParticipant_var());

  template<typename WriterOrReaderImpl>
  DDSFacade<WriterOrReaderImpl> topic(
    std::string topic_name,
    DDS::DomainParticipant_var participant = DDS::DomainParticipant_var())
  {
    typedef WriterOrReaderImpl::typesupportimpl_type typesupportimpl_type;
    typedef typesupportimpl_type::typesupport_type typesupport_type;
    typedef typesupportimpl_type::typesupport_var typesupport_var;

    determine_participant(participant);

    typesupport_var ts(new typesupportimpl_type);
    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      throw std::runtime_error(" ERROR: register_type failed!");
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic(topic_name.c_str(),
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      std::string message = "ERROR: could not create topic \"";
      message += topic_name;
      message += "\" of type \"";
      message += type_name;
      message += "\"!";
      throw std::runtime_error(message);
    }

    return DDSFacade<WriterOrReaderImpl>(participant, topic);
  }

  // either cleans up the specified participant, or all participants and the domain participant factory
  void cleanup(DDS::DomainParticipant_var participant = DDS::DomainParticipant_var());

  template<typename Writer_var>
  static DDS::DataWriter_var datawriter(const Writer_var& writer)
  {
    return DDS::DataWriter::_duplicate(writer.in());
  }

  // cleanup all participants and shutdown dds (DDSApp will no longer be usable)
  void shutdown();

private:
  DDSApp(const DDSApp& rhs);
  DDSApp& operator=(DDSApp& rhs);

  DDS::DomainParticipantFactory_var domain_participant_factory();
  void add(const DDS::DomainParticipant_var& participant);
  void remove(const DDS::DomainParticipant_var& participant);
  void determine_participant(DDS::DomainParticipant_var& participant);
  void assign_default_domain_id(::DDS::DomainId_t id);

  int argc_;
  ACE_TCHAR** argv_;
  bool domain_id_defaulted_;
  ::DDS::DomainId_t default_domain_id_;
  // don't use this var, use domain_participant_factory() to allow lazy initialization
  DDS::DomainParticipantFactory_var dpf_dont_use_;
  DDS::DomainParticipant_var default_participant_;
  Participants participants_;
  bool shutdown_;
};

} // End namespaces

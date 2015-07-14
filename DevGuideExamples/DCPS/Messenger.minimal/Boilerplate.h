/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MessengerTypeSupportImpl.h"

// Boilerplate code pulled out of mains of publisher and subscriber
// in order to simplify this example.
namespace examples { namespace boilerplate {

DDS::DomainParticipant_var
createParticipant(DDS::DomainParticipantFactory_var dpf);

DDS::Publisher_var  createPublisher(DDS::DomainParticipant_var participant);
DDS::Subscriber_var createSubscriber(DDS::DomainParticipant_var participant);

DDS::Topic_var createTopic(DDS::DomainParticipant_var participant);

DDS::DataWriter_var createDataWriter(DDS::Publisher_var publisher,
                                     DDS::Topic_var topic);
DDS::DataReader_var createDataReader(DDS::Subscriber_var subscriber,
                                     DDS::Topic_var topic,
                                     DDS::DataReaderListener_var listener);

Messenger::MessageDataWriter_var narrow(DDS::DataWriter_var writer);
Messenger::MessageDataReader_var narrow(DDS::DataReader_var reader);
Messenger::MessageDataWriter_var narrow(DDS::DataWriter_ptr writer);
Messenger::MessageDataReader_var narrow(DDS::DataReader_ptr reader);

void cleanup(DDS::DomainParticipant_var participant,
             DDS::DomainParticipantFactory_var dpf);

} } // End namespaces

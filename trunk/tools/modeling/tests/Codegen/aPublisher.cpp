
#include <iostream>
#include "model/CodegenTraits.h"
#include <model/Sync.h>

int
ACE_TMAIN( int argc, ACE_TCHAR** argv)
{
  int status = 0;

  try {
    OpenDDS::Model::Application application(argc, argv);
    Codegen::DefaultCodegenType modelinstance1(application, argc, argv);
    Codegen::DefaultCodegenType modelinstance2(application, argc, argv);

    using OpenDDS::Model::Codegen::Elements;

    Codegen::type1 data;
    data.key  = 42;
    data.name = "fortytwo";
    data.value = 236;
    data.bseq.length( 2);
    data.bseq[ 0] = 47;
    data.bseq[ 1] = 76;

    DDS::DataWriterQos writerQos;
    modelinstance1.writer( Elements::DataWriters::writer1)->get_qos( writerQos);

    DDS::DataReaderQos readerQos;
    modelinstance1.reader( Elements::DataReaders::reader1)->get_qos( readerQos);

    DDS::PublisherQos publisherQos;
    modelinstance1.publisher( Elements::Publishers::pub1)->get_qos( publisherQos);

    DDS::SubscriberQos subscriberQos;
    modelinstance1.subscriber( Elements::Subscribers::sub1)->get_qos( subscriberQos);

    DDS::TopicQos topicQos;
    dynamic_cast<DDS::Topic*>(modelinstance1.topic(Elements::Participants::part1, Elements::Topics::Codegen__topic1).ptr())->get_qos(topicQos);

    DDS::DomainParticipantQos participantQos;
    modelinstance1.participant( Elements::Participants::part1)->get_qos( participantQos);

    DDS::DataWriter_var writer2 = modelinstance1.writer( Elements::DataWriters::writer2);
    writer2->get_qos( writerQos);

    DDS::DataWriter_var writer3 = modelinstance2.writer( Elements::DataWriters::writer1);
    writer3->get_qos( writerQos);

  } catch( const std::exception& ex) {
    std::cerr << "test: caught exception in main() - " << ex.what() << std::endl;
    status = -1;
  }

  return status;
}



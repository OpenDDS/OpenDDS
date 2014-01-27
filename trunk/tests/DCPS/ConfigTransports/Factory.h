#ifndef FACTORY_H
#define FACTORY_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsTypeSupportExtC.h"

class Options;

class Factory
{
public:
  Factory(const Options& opts, const OpenDDS::DCPS::TypeSupport_var& typsup);
  virtual ~Factory();

  DDS::DomainParticipant_var participant(const DDS::DomainParticipantFactory_var& dpf) const;

  DDS::Topic_var topic(const DDS::DomainParticipant_var& dp) const;

  DDS::Subscriber_var subscriber(const DDS::DomainParticipant_var& dp) const;

  DDS::DataReader_var reader(const DDS::Subscriber_var& sub, const DDS::Topic_var& topic, const DDS::DataReaderListener_var& drl) const;

  DDS::Publisher_var publisher(const DDS::DomainParticipant_var& dp) const;

  DDS::DataWriter_var writer(const DDS::Publisher_var& pub, const DDS::Topic_var& topic, const DDS::DataWriterListener_var& dwl) const;

private:
  const Options& opts_;
  const OpenDDS::DCPS::TypeSupport_var typsup_;

};
#endif

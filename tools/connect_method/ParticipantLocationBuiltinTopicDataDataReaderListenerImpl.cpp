// -*- C++ -*-
//
// $Id: ParticipantLocationBuiltinTopicDataDataReaderListenerImpl.cpp,v 1.1 2006/03/31 16:53:17 don Exp $
//
// (c) Copyright 2008, Object Computing, Inc.
// All Rights Reserved.
//

#include "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl.h"
//INSERT CODE HERE:
//  Include the header file for the Publication's Built-in Topic
//@REMOVE_FOR_EXERCISE@
#include <dds/DdsDcpsCoreTypeSupportC.h>
//@REMOVE_FOR_EXERCISE@
#include <ace/streams.h>
#include <string>

// Implementation skeleton constructor
ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::ParticipantLocationBuiltinTopicDataDataReaderListenerImpl()
{
}

// Implementation skeleton destructor
ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::~ParticipantLocationBuiltinTopicDataDataReaderListenerImpl()
{
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
	// 1.  Narrow the DataReader to an ParticipantLocationBuiltinTopicDataDataReader
	// 2.  Read the samples from the data reader
	// 3.  Print out the contents of the samples
	DDS::ParticipantLocationBuiltinTopicDataDataReader_var builtin_dr =
		DDS::ParticipantLocationBuiltinTopicDataDataReader::_narrow(reader);
	if (0 == builtin_dr)
	{
		std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
			<< "on_data_available: _narrow failed." << std::endl;
		ACE_OS::exit(1);
	}

	DDS::ParticipantLocationBuiltinTopicData participant;
	DDS::SampleInfo si;
	DDS::ReturnCode_t status = builtin_dr->read_next_sample(participant, si);

	while (status == DDS::RETCODE_OK)
	{
		if (si.valid_data) {
			if (si.valid_data) {
				std::string loc = "";
				bool flag = false;
				if (participant.location & 1) {
					loc += "local ";
					flag = true;
				}
				if (participant.location & 2) {
					if (flag)
						loc += "& ";
					loc += "ice ";
					flag = true;
				}
				if (participant.location & 4) {
					if (flag)
						loc += "& ";
					loc += "relay";
				}

//				    << "  part: " << participant.key.value[0] << ","
//					<< participant.key.value[1] << ","
//					<< participant.key.value[2] << std::endl

				std::cout << "== Participant Location ==" << std::endl;
				std::cout 
					<< "  guid: " << participant.guid << std::endl
					<< "  addr: " << participant.local_addr << std::endl
					<< "   loc: " << loc << std::endl
					<< "  time: " << participant.local_timestamp << std::endl;
			}
		}
		status = builtin_dr->read_next_sample(participant, si);
	}
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_requested_deadline_missed(
	DDS::DataReader_ptr,
	const DDS::RequestedDeadlineMissedStatus &)
{
	std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
		<< "on_requested_deadline_missed" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_requested_incompatible_qos(
	DDS::DataReader_ptr,
	const DDS::RequestedIncompatibleQosStatus &)
{
	std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
		<< "on_requested_incompatible_qos" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_liveliness_changed(
	DDS::DataReader_ptr,
	const DDS::LivelinessChangedStatus&)
{
	std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
		<< "on_liveliness_changed" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_subscription_matched(
	DDS::DataReader_ptr,
	const DDS::SubscriptionMatchedStatus &)
{
	std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
		<< "on_subscription_matched" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_sample_rejected(
	DDS::DataReader_ptr,
	const DDS::SampleRejectedStatus&)
{
	std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
		<< "on_sample_rejected" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_sample_lost(
	DDS::DataReader_ptr,
	const DDS::SampleLostStatus&)
{
	std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
		<< "on_sample_lost" << std::endl;
}

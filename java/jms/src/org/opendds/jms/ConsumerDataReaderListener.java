package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.Message;

import DDS.ALIVE_INSTANCE_STATE;
import DDS.DataReader;
import DDS.LivelinessChangedStatus;
import DDS.NEW_VIEW_STATE;
import DDS.NOT_READ_SAMPLE_STATE;
import DDS.RETCODE_OK;
import DDS.ReadCondition;
import DDS.RequestedDeadlineMissedStatus;
import DDS.RequestedIncompatibleQosStatus;
import DDS.SampleInfo;
import DDS.SampleInfoSeqHolder;
import DDS.SampleLostStatus;
import DDS.SampleRejectedStatus;
import DDS.SubscriptionMatchStatus;
import DDS._DataReaderListenerLocalBase;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessagePayloadDataReader;
import OpenDDS.JMS.MessagePayloadDataReaderHelper;
import OpenDDS.JMS.MessagePayloadSeqHolder;

import static org.opendds.jms.MessageFactory.buildMessageFromPayload;

public class ConsumerDataReaderListener extends _DataReaderListenerLocalBase {
    private TopicMessageConsumerImpl consumer;

    public ConsumerDataReaderListener(TopicMessageConsumerImpl consumer) {
        this.consumer = consumer;
    }

    public void on_requested_deadline_missed(DataReader dataReader, RequestedDeadlineMissedStatus requestedDeadlineMissedStatus) {
        // TODO
    }

    public void on_requested_incompatible_qos(DataReader dataReader, RequestedIncompatibleQosStatus requestedIncompatibleQosStatus) {
        // TODO
    }

    public void on_sample_rejected(DataReader dataReader, SampleRejectedStatus sampleRejectedStatus) {
        // TODO
    }

    public void on_liveliness_changed(DataReader dataReader, LivelinessChangedStatus livelinessChangedStatus) {
        // TODO
    }

    public void on_data_available(DataReader dataReader) {
        MessagePayloadDataReader reader = MessagePayloadDataReaderHelper.narrow(dataReader);
        ReadCondition readCondition = reader.create_readcondition(NOT_READ_SAMPLE_STATE.value,
            NEW_VIEW_STATE.value, ALIVE_INSTANCE_STATE.value);
        MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
        SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
        int rc = reader.read_w_condition(payloads, infos, 1, readCondition);

        if (rc != RETCODE_OK.value) {
            reader.delete_readcondition(readCondition);
            return;
        }

        MessagePayload messagePayload = payloads.value[0];
        SampleInfo sampleInfo = infos.value[0];
        int handle = sampleInfo.instance_handle;
        reader.delete_readcondition(readCondition);
        Message message = buildMessageFromPayload(messagePayload, handle);

        try {
            consumer.getMessageListener().onMessage(message);
        } catch (JMSException e) {
            // TODO
        }

    }

    public void on_subscription_match(DataReader dataReader, SubscriptionMatchStatus subscriptionMatchStatus) {
        // TODO
    }

    public void on_sample_lost(DataReader dataReader, SampleLostStatus sampleLostStatus) {
        // TODO
    }
}

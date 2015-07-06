/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.JMSException;

import DDS.ANY_INSTANCE_STATE;
import DDS.ANY_VIEW_STATE;
import DDS.DataReader;
import DDS.LENGTH_UNLIMITED;
import DDS.LivelinessChangedStatus;
import DDS.NOT_READ_SAMPLE_STATE;
import DDS.RETCODE_OK;
import DDS.ReadCondition;
import DDS.RequestedDeadlineMissedStatus;
import DDS.RequestedIncompatibleQosStatus;
import DDS.SampleInfo;
import DDS.SampleInfoSeqHolder;
import DDS.SampleLostStatus;
import DDS.SampleRejectedStatus;
import DDS.SubscriptionMatchedStatus;
import DDS._DataReaderListenerLocalBase;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessagePayloadDataReader;
import OpenDDS.JMS.MessagePayloadDataReaderHelper;
import OpenDDS.JMS.MessagePayloadSeqHolder;

import static org.opendds.jms.ConsumerMessageFactory.buildMessageFromPayload;

/**
 * @author  Weiqi Gao
 */
public class ConsumerDataReaderListener extends _DataReaderListenerLocalBase {
    private MessageConsumerImpl consumer;
    private SessionImpl sessionImpl;

    public ConsumerDataReaderListener(MessageConsumerImpl consumer, SessionImpl sessionImpl) {
        this.consumer = consumer;
        this.sessionImpl = sessionImpl;
    }

    public void on_requested_deadline_missed(DataReader dataReader, RequestedDeadlineMissedStatus requestedDeadlineMissedStatus) {
        // No-op
    }

    public void on_requested_incompatible_qos(DataReader dataReader, RequestedIncompatibleQosStatus requestedIncompatibleQosStatus) {
        // No-op
    }

    public void on_sample_rejected(DataReader dataReader, SampleRejectedStatus sampleRejectedStatus) {
        // No-op
    }

    public void on_liveliness_changed(DataReader dataReader, LivelinessChangedStatus livelinessChangedStatus) {
        // No-op
    }

    public void on_data_available(DataReader dataReader) {
        MessagePayloadDataReader reader = MessagePayloadDataReaderHelper.narrow(dataReader);
        MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
        SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);

        if (!readOneSample(reader, payloads, infos)) return;

        int length = payloads.value.length;
        for (int i = 0; i < length; i++) {
            MessagePayload messagePayload = payloads.value[i];
            SampleInfo sampleInfo = infos.value[i];
            int handle = sampleInfo.instance_handle;
            AbstractMessageImpl message = buildMessageFromPayload(messagePayload, handle, sessionImpl);
            try {
                if (consumer.isDurableAcknowledged(message)) continue;

            } catch (JMSException e) {
                throw new IllegalStateException(e);
            }
            DataReaderHandlePair dataReaderHandlePair = new DataReaderHandlePair(reader, handle);
            sessionImpl.getMessageDeliveryExecutor().execute(new MessageDispatcher(message, dataReaderHandlePair, consumer, sessionImpl));
        }
    }

    private static boolean readOneSample(MessagePayloadDataReader reader, MessagePayloadSeqHolder payloads, SampleInfoSeqHolder infos) {
        ReadCondition readCondition = reader.create_querycondition(NOT_READ_SAMPLE_STATE.value,
            ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value, "ORDER BY theHeader.TwentyMinusJMSPriority", new String[] {});
        int rc = reader.read_w_condition(payloads, infos, LENGTH_UNLIMITED.value, readCondition);
        reader.delete_readcondition(readCondition);
        return rc == RETCODE_OK.value;
    }

    public void on_subscription_matched(DataReader dataReader, SubscriptionMatchedStatus subscriptionMatchStatus) {
        // No-op
    }

    public void on_sample_lost(DataReader dataReader, SampleLostStatus sampleLostStatus) {
        // No-op
    }
}

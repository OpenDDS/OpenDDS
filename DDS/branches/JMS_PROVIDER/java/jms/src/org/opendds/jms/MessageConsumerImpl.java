/*
 * $Id$
 */

package org.opendds.jms;

import java.util.List;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Session;

import DDS.ALIVE_INSTANCE_STATE;
import DDS.DATAREADER_QOS_DEFAULT;
import DDS.DATA_AVAILABLE_STATUS;
import DDS.DURATION_INFINITY_NSEC;
import DDS.DURATION_INFINITY_SEC;
import DDS.DataReader;
import DDS.DataReaderQosHolder;
import DDS.Duration_t;
import DDS.GuardCondition;
import DDS.NEW_VIEW_STATE;
import DDS.NOT_NEW_VIEW_STATE;
import DDS.NOT_READ_SAMPLE_STATE;
import DDS.READ_SAMPLE_STATE;
import DDS.RETCODE_OK;
import DDS.RETCODE_TIMEOUT;
import DDS.ReadCondition;
import DDS.ReadConditionHelper;
import DDS.SampleInfo;
import DDS.SampleInfoSeqHolder;
import DDS.Subscriber;
import DDS.Topic;
import DDS.WaitSet;
import DDS.DataReaderQos;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessagePayloadDataReader;
import OpenDDS.JMS.MessagePayloadDataReaderHelper;
import OpenDDS.JMS.MessagePayloadSeqHolder;

import static org.opendds.jms.ConsumerMessageFactory.buildMessageFromPayload;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.DataReaderQosPolicy;

/**
 * @author  Weiqi Gao
 * @version $Revision$
 */
public class MessageConsumerImpl implements MessageConsumer {
    private ConnectionImpl connection;
    private Destination destination;
    private String messageSelector;
    private MessageListener messageListener;

    // DDS related stuff
    private Subscriber subscriber;
    private MessagePayloadDataReader messagePayloadDataReader;

    private boolean closed;
    private final WaitSet waitSet;
    private final GuardCondition closeToken;

    // JMS 1.1, 4.4.11,
    private SessionImpl sessionImpl;
    private List<DataReaderHandlePair> toBeRecovered;

    private MessageDeliveryHelper helper;

    public MessageConsumerImpl(SessionImpl sessionImpl, Destination destination, String messageSelector, boolean noLocal) throws JMSException {
        this.sessionImpl = sessionImpl;
        this.destination = destination;
        this.messageSelector = messageSelector;
        this.connection = sessionImpl.getOwningConnection();

        if (noLocal) {
            subscriber = connection.getRemoteSubscriber();
        } else {
            subscriber = connection.getLocalSubscriber();
        }

        this.closed = false;
        this.messagePayloadDataReader = fromDestination(destination, subscriber);
        this.closeToken = new GuardCondition();
        this.closeToken.set_trigger_value(false);
        this.waitSet = new WaitSet();
        this.waitSet.attach_condition(closeToken);

        this.toBeRecovered = null;

        this.helper = new MessageDeliveryHelper(connection);
    }

    private MessagePayloadDataReader fromDestination(Destination destination, Subscriber subscriber) throws JMSException {
        Logger logger = sessionImpl.getOwningConnection().getLogger();
        DDS.Topic ddsTopic = extractDDSTopicFromDestination(destination);
        DataReaderQosPolicy dataReaderQosPolicy = ((TopicImpl) destination).getDataReaderQosPolicy();

        DataReaderQosHolder qosHolder = new DataReaderQosHolder(DATAREADER_QOS_DEFAULT.get());
        subscriber.get_default_datareader_qos(qosHolder);

        DataReaderQos readerQos = qosHolder.value;
        if (dataReaderQosPolicy != null) {
            dataReaderQosPolicy.setQos(readerQos);
        }

        DataReader reader = subscriber.create_datareader(ddsTopic, readerQos, null);

        logger.debug("Created %s %s", reader, readerQos);

        return MessagePayloadDataReaderHelper.narrow(reader);
    }

    private Topic extractDDSTopicFromDestination(Destination destination) throws JMSException {
        // TODO placeholder, to be elaborated
        TopicImpl topicImpl = (TopicImpl) destination;
        return topicImpl.createDDSTopic(sessionImpl.getOwningConnection());
    }

    public String getMessageSelector() throws JMSException {
        return Strings.isEmpty(messageSelector) ? null : messageSelector;
    }

    public MessageListener getMessageListener() {
        return messageListener;
    }

    public void setMessageListener(MessageListener messageListener) throws JMSException {
        checkClosed();
        if (messageListener == null) {
            messagePayloadDataReader.set_listener(null, 0);
        } else {
            messagePayloadDataReader.set_listener(new ConsumerDataReaderListener(this, sessionImpl), DATA_AVAILABLE_STATUS.value);
        }
        this.messageListener = messageListener;
    }

    public Message receive() throws JMSException {
        checkClosed();
        Duration_t duration = new Duration_t(DURATION_INFINITY_SEC.value, DURATION_INFINITY_NSEC.value);
        return doRecoverOrReceive(duration);
    }

    public Message receive(long timeout) throws JMSException {
        checkClosed();
        Duration_t duration = new Duration_t();
        if (timeout < 0) throw new IllegalArgumentException("The timeout specified is negative: " + timeout);
        if (timeout == 0) {
            duration.sec = DURATION_INFINITY_SEC.value;
            duration.nanosec = DURATION_INFINITY_NSEC.value;
        } else {
            duration.sec = (int) timeout / 1000;
            duration.nanosec = ((int) (timeout % 1000)) * 1000;
        }
        return doRecoverOrReceive(duration);
    }

    public Message receiveNoWait() throws JMSException {
        checkClosed();
        Duration_t duration = new Duration_t(0, 0);
        return doRecoverOrReceive(duration);
    }

    private Message doRecoverOrReceive(Duration_t duration) throws JMSException {
        Message message = null;

        helper.lock();
        try {
            helper.awaitStart();
            helper.notifyBusy();

            if (toBeRecovered != null && !toBeRecovered.isEmpty()) {
                message = doRecoverSync();
            } else {
                message = doReceive(duration);
            }

        } catch (InterruptedException e) {
        } finally {
            helper.notifyIdle();
            helper.unlock();
        }
        return message;
    }

    private Message doReceive(Duration_t duration) throws JMSException {
        ReadCondition readCondition = messagePayloadDataReader.create_querycondition(NOT_READ_SAMPLE_STATE.value,
                NEW_VIEW_STATE.value, ALIVE_INSTANCE_STATE.value, "ORDER BY theHeader.TwentyMinusJMSPriority", new String[] {});
        int rc = waitSet.attach_condition(readCondition);
        if (rc != RETCODE_OK.value) {
            throw new JMSException("Cannot attach readCondition to OpenDDS WaitSet.");
        }

        DDS.ConditionSeqHolder conditions = new DDS.ConditionSeqHolder(new DDS.Condition[0]);

        while (true) {
            rc = waitSet.wait(conditions, duration);
            if (rc == RETCODE_TIMEOUT.value) {
                waitSet.detach_condition(readCondition);
                messagePayloadDataReader.delete_readcondition(readCondition);
                return null;
            } else if (rc != RETCODE_OK.value) {
                waitSet.detach_condition(readCondition);
                messagePayloadDataReader.delete_readcondition(readCondition);
                throw new JMSException("WaitSet returned from wait() .");
            }

            for (int i = 0; i < conditions.value.length; i++) {
                final DDS.Condition innerCondition = conditions.value[i];
                if (innerCondition._is_equivalent(readCondition)) {
                    ReadCondition innerReadCondition = ReadConditionHelper.narrow(innerCondition);
                    MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
                    SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
                    rc = messagePayloadDataReader.read_w_condition(payloads, infos, 1, innerReadCondition);
                    if (rc != RETCODE_OK.value) continue;

                    // We should see one sample
                    MessagePayload messagePayload = payloads.value[0];
                    SampleInfo sampleInfo = infos.value[0];
                    int handle = sampleInfo.instance_handle;
                    waitSet.detach_condition(readCondition);
                    messagePayloadDataReader.delete_readcondition(readCondition);
                    AbstractMessageImpl message = buildMessageFromPayload(messagePayload, handle, sessionImpl);
                    DataReaderHandlePair dataReaderHandlePair = new DataReaderHandlePair(messagePayloadDataReader, handle);
                    sessionImpl.addToUnacknowledged(dataReaderHandlePair, this);
                    if (sessionImpl.getAcknowledgeMode() != Session.CLIENT_ACKNOWLEDGE) {
                        sessionImpl.doAcknowledge();
                    }
                    return message;
                } else if (innerCondition._is_equivalent(closeToken)) {
                    // close() is called from another thread
                    waitSet.detach_condition(readCondition);
                    messagePayloadDataReader.delete_readcondition(readCondition);
                    return null;
                }
            }
        }
    }

    public void close() throws JMSException {
        if (closed) return;
        if (messagePayloadDataReader != null) {
            closeToken.set_trigger_value(true);
            subscriber.delete_datareader(messagePayloadDataReader);
        }
        helper.release();
        this.closed = true;
    }

    private void checkClosed() {
        // JMS 1.1, 4.4.1
        if (closed) throw new IllegalStateException("This MessageConsumer is closed.");
    }

    void doRecover(List<DataReaderHandlePair> dataReaderHandlePairs) {
        if (messageListener != null) {
            doRecoverAsync(dataReaderHandlePairs);
        } else {
            toBeRecovered = dataReaderHandlePairs;
        }
    }

    private void doRecoverAsync(List<DataReaderHandlePair> dataReaderHandlePairs) {
        for (DataReaderHandlePair pair : dataReaderHandlePairs) {
            final MessagePayloadDataReader reader = pair.getDataReader();
            final int handle = pair.getInstanceHandle();
            MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
            SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
            int rc = reader.read_instance(payloads, infos, 1, handle, READ_SAMPLE_STATE.value, NOT_NEW_VIEW_STATE.value, ALIVE_INSTANCE_STATE.value);
            if (rc != RETCODE_OK.value) continue;

            MessagePayload messagePayload = payloads.value[0];
            SampleInfo sampleInfo = infos.value[0];
            AbstractMessageImpl message = buildMessageFromPayload(messagePayload, handle, sessionImpl);
            message.setJMSRedelivered(true);
            sessionImpl.getMessageDeliveryExecutor().execute(new MessageDispatcher(message, pair, this, sessionImpl));
        }
    }

    private Message doRecoverSync() {
        while (true) {
            final DataReaderHandlePair pair = toBeRecovered.remove(0);
            final MessagePayloadDataReader reader = pair.getDataReader();
            final int handle = pair.getInstanceHandle();
            MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
            SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
            int rc = reader.read_instance(payloads, infos, 1, handle, READ_SAMPLE_STATE.value, NOT_NEW_VIEW_STATE.value, ALIVE_INSTANCE_STATE.value);
            if (rc != RETCODE_OK.value) continue;
            MessagePayload messagePayload = payloads.value[0];
            SampleInfo sampleInfo = infos.value[0];
            AbstractMessageImpl message = buildMessageFromPayload(messagePayload, handle, sessionImpl);
            message.setJMSRedelivered(true);
            sessionImpl.addToUnacknowledged(pair, this);
            if (sessionImpl.getAcknowledgeMode() != Session.CLIENT_ACKNOWLEDGE) {
                sessionImpl.doAcknowledge();
            }
            return message;
        }
    }
}

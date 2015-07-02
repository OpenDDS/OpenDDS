/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.ArrayList;
import java.util.List;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Session;

import DDS.ANY_INSTANCE_STATE;
import DDS.ANY_SAMPLE_STATE;
import DDS.ANY_VIEW_STATE;
import DDS.DATA_AVAILABLE_STATUS;
import DDS.DURATION_INFINITE_NSEC;
import DDS.DURATION_INFINITE_SEC;
import DDS.DataReader;
import DDS.DataReaderQosHolder;
import DDS.Duration_t;
import DDS.GuardCondition;
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
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessagePayloadDataReader;
import OpenDDS.JMS.MessagePayloadDataReaderHelper;
import OpenDDS.JMS.MessagePayloadSeqHolder;

import static org.opendds.jms.ConsumerMessageFactory.buildMessageFromPayload;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.DataReaderQosPolicy;
import org.opendds.jms.qos.QosPolicies;

/**
 * @author  Weiqi Gao
 */
public class MessageConsumerImpl implements MessageConsumer {
    private Logger logger;
    protected ConnectionImpl connection;
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

    // JMS 1.1, 4.4.11, The sessions view of unacknowledged Messages, used in recover()
    private List<DataReaderHandlePair> unacknowledged;
    private final Object lockForUnacknowledged;

    public MessageConsumerImpl(SessionImpl sessionImpl, Destination destination, String messageSelector, boolean noLocal) throws JMSException {
        this.sessionImpl = sessionImpl;
        this.destination = destination;
        this.messageSelector = messageSelector;
        this.connection = sessionImpl.getOwningConnection();

        this.logger = connection.getLogger();

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

        this.unacknowledged = new ArrayList<DataReaderHandlePair>();
        this.lockForUnacknowledged = new Object();
    }

    private MessagePayloadDataReader fromDestination(Destination destination, Subscriber subscriber) throws JMSException {
        DDS.Topic ddsTopic = extractDDSTopicFromDestination(destination);
        DataReaderQosPolicy dataReaderQosPolicy = ((TopicImpl) destination).getDataReaderQosPolicy();

        DataReaderQosHolder holder =
            new DataReaderQosHolder(QosPolicies.newDataReaderQos());
        subscriber.get_default_datareader_qos(holder);

        dataReaderQosPolicy.setQos(holder.value);

        DataReader reader = subscriber.create_datareader(ddsTopic, holder.value, null, 0);
        logger.debug("Created %s -> %s", reader, dataReaderQosPolicy);

        return MessagePayloadDataReaderHelper.narrow(reader);
    }

    private Topic extractDDSTopicFromDestination(Destination destination) throws JMSException {
        TopicImpl topicImpl = (TopicImpl) destination;
        return topicImpl.createDDSTopic(sessionImpl.getOwningConnection());
    }

    protected Destination getDestination() {
        return destination;
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
        Duration_t duration = new Duration_t(DURATION_INFINITE_SEC.value, DURATION_INFINITE_NSEC.value);
        return doRecoverOrReceive(duration);
    }

    public Message receive(long timeout) throws JMSException {
        checkClosed();
        Duration_t duration = new Duration_t();
        if (timeout < 0) throw new IllegalArgumentException("The timeout specified is negative: " + timeout);
        if (timeout == 0) {
            duration.sec = DURATION_INFINITE_SEC.value;
            duration.nanosec = DURATION_INFINITE_NSEC.value;
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
            ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value, "ORDER BY theHeader.TwentyMinusJMSPriority", new String[]{});
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
                    if (isDurableAcknowledged(message)) continue;
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

        logger.debug("Closing %s", this);

        if (messagePayloadDataReader != null) {
            closeToken.set_trigger_value(true);
            subscriber.delete_datareader(messagePayloadDataReader);
        }
        doDurableClose();
        helper.release();
        this.closed = true;
    }

    protected void doDurableClose() {
        // No-op for non-durable subscriptions
    }

    private void checkClosed() {
        // JMS 1.1, 4.4.1
        if (closed) throw new IllegalStateException("This MessageConsumer is closed.");
    }

    void doRecover() throws JMSException {
        List<DataReaderHandlePair> dataReaderHandlePairs = null;
        synchronized (lockForUnacknowledged) {
            dataReaderHandlePairs = new ArrayList<DataReaderHandlePair>(unacknowledged);
            unacknowledged.clear();
        }
        if (messageListener != null) {
            doRecoverAsync(dataReaderHandlePairs);
        } else {
            toBeRecovered = dataReaderHandlePairs;
        }
    }

    private void doRecoverAsync(List<DataReaderHandlePair> dataReaderHandlePairs) throws JMSException {
        for (DataReaderHandlePair pair : dataReaderHandlePairs) {
            final MessagePayloadDataReader reader = pair.getDataReader();
            final int handle = pair.getInstanceHandle();
            MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
            SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
            int rc = reader.read_instance(payloads, infos, 1, handle, READ_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);
            if (rc != RETCODE_OK.value) continue;

            MessagePayload messagePayload = payloads.value[0];
            SampleInfo sampleInfo = infos.value[0];
            AbstractMessageImpl message = buildMessageFromPayload(messagePayload, handle, sessionImpl);
            if (isDurableAcknowledged(message)) continue;
            message.setJMSRedelivered(true);
            sessionImpl.getMessageDeliveryExecutor().execute(new MessageDispatcher(message, pair, this, sessionImpl));
        }
    }

    private Message doRecoverSync() throws JMSException {
        while (true) {
            final DataReaderHandlePair pair = toBeRecovered.remove(0);
            final MessagePayloadDataReader reader = pair.getDataReader();
            final int handle = pair.getInstanceHandle();
            MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
            SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
            int rc = reader.read_instance(payloads, infos, 1, handle, READ_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);
            if (rc != RETCODE_OK.value) continue;
            MessagePayload messagePayload = payloads.value[0];
            SampleInfo sampleInfo = infos.value[0];
            AbstractMessageImpl message = buildMessageFromPayload(messagePayload, handle, sessionImpl);
            if (isDurableAcknowledged(message)) continue;
            message.setJMSRedelivered(true);
            sessionImpl.addToUnacknowledged(pair, this);
            if (sessionImpl.getAcknowledgeMode() != Session.CLIENT_ACKNOWLEDGE) {
                sessionImpl.doAcknowledge();
            }
            return message;
        }
    }

    protected boolean isDurableAcknowledged(AbstractMessageImpl message) throws JMSException {
        return false;
    }

    public void doAcknowledge() throws JMSException {
        List<DataReaderHandlePair> copy;
        synchronized(lockForUnacknowledged) {
            copy = new ArrayList<DataReaderHandlePair>(unacknowledged);
            unacknowledged.clear();
        }
        for (DataReaderHandlePair pair : copy) {
            MessagePayloadDataReader dataReader = pair.getDataReader();
            int instanceHandle = pair.getInstanceHandle();

            MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
            SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
            int rc = dataReader.take_instance(payloads, infos, 1, instanceHandle, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);
            if (rc == RETCODE_OK.value) {
                AbstractMessageImpl message = buildMessageFromPayload(payloads.value[0], instanceHandle, sessionImpl);
                doDurableAcknowledge(message);
            } else {
                // The instance handle is gone from the dataReader for some other reason.
            }
        }
    }

    protected void doDurableAcknowledge(Message message) {
        // No-op for non-durable subscriptions
    }

    public void addToUnacknowledged(DataReaderHandlePair dataReaderHandlePair) {
        unacknowledged.add(dataReaderHandlePair);
    }

}

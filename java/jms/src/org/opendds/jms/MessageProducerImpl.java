/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.HashMap;
import java.util.Map;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;

import DDS.DURATION_INFINITE_NSEC;
import DDS.DURATION_INFINITE_SEC;
import DDS.DataWriterQosHolder;
import DDS.HANDLE_NIL;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessagePayloadDataWriter;

import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.QosPolicies;

/**
 * @author  Weiqi Gao
 */
public class MessageProducerImpl implements MessageProducer {
    private Logger logger;
    private SessionImpl session;
    private Destination destination;
    private boolean disableMessageID;
    private boolean disableMessageTimestamp;
    private int deliveryMode;
    private int priority;
    private long timeToLive;

    // DDS related stuff
    private final DataWriterPair dataWriterPair; // For identified (constructed with a non-null Destination) MessageProducer
    private final Map<Destination, DataWriterPair> dataWriterPairMap;

    private boolean closed;

    public MessageProducerImpl(SessionImpl session, Destination destination) throws JMSException {
        this.session = session;
        this.logger = session.getOwningConnection().getLogger();
        this.destination = destination;

        initProducer();
        if (destination != null) {
            this.dataWriterPair = DataWriterPair.fromDestination(session, destination);
            this.dataWriterPairMap = null;
        } else {
            this.dataWriterPair = null;
            this.dataWriterPairMap = new HashMap<Destination, DataWriterPair>();
        }
    }

    private void initProducer() {
        this.disableMessageID = false;
        this.disableMessageTimestamp = false;
        this.deliveryMode = DeliveryMode.PERSISTENT;
        this.priority = 4;
        this.timeToLive = 0;
        this.closed = false;
    }

    public Destination getDestination() throws JMSException {
        return destination;
    }

    public boolean getDisableMessageID() throws JMSException {
        return disableMessageID;
    }

    public void setDisableMessageID(boolean disableMessageID) throws JMSException {
        // No-op, may be enabled in a future optimazation
    }

    public boolean getDisableMessageTimestamp() throws JMSException {
        return disableMessageTimestamp;
    }

    public void setDisableMessageTimestamp(boolean disableMessageTimestamp) throws JMSException {
        // No-op, may be enabled in a future optimization
    }

    public int getDeliveryMode() throws JMSException {
        return deliveryMode;
    }

    public void setDeliveryMode(int deliveryMode) throws JMSException {
        checkClosed();
        validateDeliveryMode(deliveryMode);
        this.deliveryMode = deliveryMode;
    }

    private void validateDeliveryMode(int deliveryMode) {
        if (deliveryMode != DeliveryMode.PERSISTENT && deliveryMode != DeliveryMode.NON_PERSISTENT) {
            throw new IllegalArgumentException("Illegal deliveryMode: " + deliveryMode + ".");
        }
    }

    public int getPriority() throws JMSException {
        return priority;
    }

    public void setPriority(int priority) throws JMSException {
        checkClosed();
        validatePriority(priority);
        this.priority = priority;
    }

    private void validatePriority(int priority) {
        if (priority < 0 || priority > 9) {
            throw new IllegalArgumentException("Illegal priority: " + priority + ".");
        }
    }

    public long getTimeToLive() throws JMSException {
        return timeToLive;
    }

    public void setTimeToLive(long timeToLive) throws JMSException {
        checkClosed();
        validateTimeTiLive(timeToLive);
        this.timeToLive = timeToLive;
    }

    private void validateTimeTiLive(long timeToLive) {
        if (timeToLive < 0) {
            throw new IllegalArgumentException("Illegal timeToLive: " + timeToLive + ".");
        }
    }

    public void send(Message message) throws JMSException {
        send(message, deliveryMode, priority, timeToLive);
    }

    public void send(Message message,
                     int deliveryMode,
                     int priority,
                     long timeToLive) throws JMSException {
        if (dataWriterPair == null) {
            throw new UnsupportedOperationException("This MessageProducer is created without a Destination.");
        }
        checkClosed();
        final MessagePayloadDataWriter dataWriter = dataWriterPair.getDataWriter(deliveryMode);

        validateMessage(message);
        validateDeliveryMode(deliveryMode);
        validatePriority(priority);
        validateTimeTiLive(timeToLive);
        populateMessageHeader(destination, message, deliveryMode, priority, timeToLive);
        manipulateDataWriterQoS(dataWriter, timeToLive);
        writeDataWithDataWriter(message, dataWriter);
    }

    public void send(Destination destination, Message message) throws JMSException {
        send(destination, message, deliveryMode, priority, timeToLive);
    }

    public void send(Destination destination,
                     Message message,
                     int deliveryMode,
                     int priority,
                     long timeToLive) throws JMSException {

        if (dataWriterPair != null) {
            throw new UnsupportedOperationException("This MessageProducer is created with a Destination.");
        }
        checkClosed();
        DataWriterPair dataWriterPair = DataWriterPair.fromDestination(session, destination);
        final MessagePayloadDataWriter dataWriter = dataWriterPair.getDataWriter(deliveryMode);

        validateMessage(message);
        validateDeliveryMode(deliveryMode);
        validatePriority(priority);
        validateTimeTiLive(timeToLive);
        populateMessageHeader(destination, message, deliveryMode, priority, timeToLive);
        manipulateDataWriterQoS(dataWriter, timeToLive);
        writeDataWithDataWriter(message, dataWriter);

        dataWriterPair.destroy();
    }

    private void validateMessage(Message message) {
        Objects.ensureNotNull(message);
    }

    private void populateMessageHeader(Destination destination, Message message, int deliveryMode, int priority, long timeToLive) throws JMSException {
        final String id = Strings.randomUuid();
        final long timestamp = System.currentTimeMillis();
        final long expiration = (timeToLive == 0L) ? 0L : (timestamp + timeToLive);

        message.setJMSMessageID(id);
        message.setJMSDestination(destination);
        message.setJMSDeliveryMode(deliveryMode);
        message.setJMSPriority(priority);
        message.setJMSTimestamp(timestamp);
        message.setJMSExpiration(expiration);
    }

    private void manipulateDataWriterQoS(MessagePayloadDataWriter dataWriter, long timeToLive) {
        DataWriterQosHolder holder = new DataWriterQosHolder(QosPolicies.newDataWriterQos());
        dataWriter.get_qos(holder);
        if (timeToLive == 0L) {
            holder.value.lifespan.duration.sec = DURATION_INFINITE_SEC.value;
            holder.value.lifespan.duration.nanosec = DURATION_INFINITE_NSEC.value;
        } else {
            holder.value.lifespan.duration.sec = (int) timeToLive / 1000;
            holder.value.lifespan.duration.nanosec = ((int) (timeToLive % 1000)) * 1000000;
        }
        dataWriter.set_qos(holder.value);
    }

    private void writeDataWithDataWriter(Message message, MessagePayloadDataWriter dataWriter) {
        AbstractMessageImpl messageImpl = (AbstractMessageImpl) message;
        final MessagePayload payload = messageImpl.getPayload();
        dataWriter.write(payload, HANDLE_NIL.value);

        // N.B. We must unregister the instance immediately;
        // this prevents a resource leak in OpenDDS proper when
        // volatile keys are used to identify instances.
        dataWriter.unregister_instance(payload, HANDLE_NIL.value);
    }

    public void close() throws JMSException {
        if (closed) return;

        logger.debug("Closing %s", this);

        if (dataWriterPair != null) {
            dataWriterPair.destroy();
        } else {
            for (DataWriterPair pair : dataWriterPairMap.values()) {
                pair.destroy();
            }
        }
        closed = true;
    }

    private void checkClosed() throws JMSException {
        // JMS 1.1, 4.4.1
        if (closed) throw new JMSException("This MessageProducer is closed.");
    }
}

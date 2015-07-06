/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;

import DDS.DataWriter;
import DDS.DataWriterQosHolder;
import DDS.DurabilityQosPolicyKind;
import DDS.Duration_t;
import DDS.Publisher;
import DDS.Topic;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.JMS.MessagePayloadDataWriter;
import OpenDDS.JMS.MessagePayloadDataWriterHelper;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.DataWriterQosPolicy;
import org.opendds.jms.qos.QosPolicies;

/**
 * @author  Weiqi Gao
 */
public class DataWriterPair {
    private Logger logger = Logger.getLogger(DataWriterPair.class);

    private MessagePayloadDataWriter persistentDW;
    private MessagePayloadDataWriter volatileDW;
    private Publisher publisher;

    private DataWriterPair(MessagePayloadDataWriter persistentDW, MessagePayloadDataWriter volatileDW, Publisher publisher) {
        this.persistentDW = persistentDW;
        this.volatileDW = volatileDW;
        this.publisher = publisher;
    }

    public static DataWriterPair fromDestination(SessionImpl session, Destination destination) throws JMSException {
        ConnectionImpl connection = session.getOwningConnection();
        Logger logger = connection.getLogger();

        DDS.Topic ddsTopic = extractDDSTopicFromDestination(connection, destination);
        Publisher publisher = connection.getPublisher();

        DataWriterQosPolicy dataWriterQosPolicy = ((TopicImpl) destination).getDataWriterQosPolicy();

        // Create persistent data writer
        DataWriterQosHolder holder;

        holder = new DataWriterQosHolder(QosPolicies.newDataWriterQos());
        publisher.get_default_datawriter_qos(holder);

        dataWriterQosPolicy.setQos(holder.value);

        holder.value.durability.kind = DurabilityQosPolicyKind.PERSISTENT_DURABILITY_QOS;
        final DataWriter dataWriter = publisher.create_datawriter(ddsTopic, holder.value, null, DEFAULT_STATUS_MASK.value);
        MessagePayloadDataWriter persistentDW = MessagePayloadDataWriterHelper.narrow(dataWriter);
        logger.debug("Created %s -> %s", persistentDW, dataWriterQosPolicy);

        // Create volatile data writer
        holder = new DataWriterQosHolder(QosPolicies.newDataWriterQos());
        publisher.get_default_datawriter_qos(holder);

        dataWriterQosPolicy.setQos(holder.value);

        holder.value.durability.kind = DurabilityQosPolicyKind.VOLATILE_DURABILITY_QOS;
        final DataWriter dataWriter2 = publisher.create_datawriter(ddsTopic, holder.value, null, DEFAULT_STATUS_MASK.value);
        MessagePayloadDataWriter volatileDW = MessagePayloadDataWriterHelper.narrow(dataWriter2);
        logger.debug("Created %s -> %s", volatileDW, dataWriterQosPolicy);

        return new DataWriterPair(persistentDW, volatileDW, publisher);
    }

    private static Topic extractDDSTopicFromDestination(ConnectionImpl connection, Destination destination) throws JMSException {
        TopicImpl topicImpl = (TopicImpl) destination;
        return topicImpl.createDDSTopic(connection);
    }

    public MessagePayloadDataWriter getDataWriter(int deliveryMode) {
        if (deliveryMode == DeliveryMode.PERSISTENT) return persistentDW;
        if (deliveryMode == DeliveryMode.NON_PERSISTENT) return volatileDW;
        throw new IllegalArgumentException("Illegal deliveryMode: " + deliveryMode);
    }

    public void destroy() {
        ExecutorService executor = Executors.newCachedThreadPool();

        executor.submit(new DataWriterAck(persistentDW));
        executor.submit(new DataWriterAck(volatileDW));

        executor.shutdown();
        try {
            executor.awaitTermination(30, TimeUnit.SECONDS);

        } catch (InterruptedException e) {
            logger.warn("destroy() interrupted; possible data loss!");

        } finally {
            publisher.delete_datawriter(persistentDW);
            publisher.delete_datawriter(volatileDW);
        }
    }

    //

    private static class DataWriterAck implements Runnable {
        private DataWriter writer;

        public DataWriterAck(DataWriter writer) {
            this.writer = writer;
        }

        public void run() {
            // Wait for sent messages to be acknowledged; the max_time
            // duration should eventually become configurable.
            writer.wait_for_acknowledgments(new Duration_t(30, 0));
        }
    }
}

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.Arrays;

import javax.jms.JMSException;
import javax.resource.ResourceException;

import DDS.DomainParticipant;
import DDS.Publisher;
import DDS.PublisherQosHolder;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.DCPS.transport.TransportConfig;
import OpenDDS.DCPS.transport.TheTransportRegistry;

import org.opendds.jms.common.ExceptionHelper;
import org.opendds.jms.common.PartitionHelper;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.QosPolicies;
import org.opendds.jms.resource.ConnectionRequestInfoImpl;
import org.opendds.jms.resource.ManagedConnectionImpl;
import org.opendds.jms.transport.TransportManager;

/**
 * @author  Steven Stallion
 */
public class PublisherManager {
    private ManagedConnectionImpl connection;
    private ConnectionRequestInfoImpl cxRequestInfo;
    private Publisher publisher;
    private TransportManager transportManager;

    public PublisherManager(ManagedConnectionImpl connection) throws ResourceException {
        assert connection != null;

        this.connection = connection;
        this.cxRequestInfo = connection.getConnectionRequestInfo();
        this.transportManager = new TransportManager(cxRequestInfo.getPublisherTransport());
    }

    protected Publisher createPublisher() throws JMSException {
        try {
            Logger logger = connection.getLogger();

            PublisherQosHolder holder =
                new PublisherQosHolder(QosPolicies.newPublisherQos());

            DomainParticipant participant = connection.getParticipant();
            participant.get_default_publisher_qos(holder);

            PublisherQosPolicy policy = cxRequestInfo.getPublisherQosPolicy();
            policy.setQos(holder.value);

            // Set PARTITION QosPolicy to support the noLocal client
            // specifier on created MessageConsumer instances:
            holder.value.partition = PartitionHelper.match(connection.getConnectionId());

            Publisher publisher = participant.create_publisher(holder.value, null, DEFAULT_STATUS_MASK.value);
            if (publisher == null) {
                throw new JMSException("Unable to create Publisher; please check logs");
            }
            if (logger.isDebugEnabled()) {
                logger.debug("Created %s -> %s", publisher, policy);
                logger.debug("%s using PARTITION %s", publisher, Arrays.deepToString(holder.value.partition.name));
            }

            TransportConfig transport = transportManager.getTransport();
            TheTransportRegistry.bind_config(transport, publisher);
            logger.debug("Attached %s to %s", publisher, transport);

            return publisher;

        } catch (Exception e) {
            throw ExceptionHelper.notify(connection, e);
        }
    }

    public synchronized Publisher getPublisher() throws JMSException {
        if (publisher == null) {
            publisher = createPublisher();
        }
        return publisher;
    }
}

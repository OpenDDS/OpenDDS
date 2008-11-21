/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.resource.ResourceException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import DDS.DomainParticipant;
import DDS.Publisher;
import DDS.PublisherQosHolder;
import OpenDDS.DCPS.transport.AttachStatus;
import OpenDDS.DCPS.transport.TransportImpl;

import org.opendds.jms.common.PartitionHelper;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.QosPolicies;
import org.opendds.jms.resource.ConnectionRequestInfoImpl;
import org.opendds.jms.resource.ManagedConnectionImpl;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class PublisherManager {
    private static Log log = LogFactory.getLog(PublisherManager.class);

    private ManagedConnectionImpl connection;
    private ConnectionRequestInfoImpl cxRequestInfo;
    private Publisher publisher;

    public PublisherManager(ManagedConnectionImpl connection) throws ResourceException {
        this.connection = connection;
        cxRequestInfo = connection.getConnectionRequestInfo();
    }

    protected Publisher createPublisher() throws JMSException {
        PublisherQosHolder holder =
            new PublisherQosHolder(QosPolicies.newPublisherQos());

        DomainParticipant participant = connection.getParticipant();
        participant.get_default_publisher_qos(holder);

        PublisherQosPolicy policy = cxRequestInfo.getPublisherQosPolicy();
        policy.setQos(holder.value);

        // Set PARTITION QosPolicy to support the noLocal client
        // specifier on created MessageConsumer instances:
        holder.value.partition = PartitionHelper.match(connection.getConnectionId());

        Publisher publisher = participant.create_publisher(holder.value, null);
        if (publisher == null) {
            throw new JMSException("Unable to create Publisher; please check logs");
        }
        if (log.isDebugEnabled()) {
            log.debug(String.format("Created %s using %s", publisher, holder.value));
        }

        TransportImpl transport = cxRequestInfo.getPublisherTransport();
        if (transport.attach_to_publisher(publisher).value() != AttachStatus._ATTACH_OK) {
            throw new JMSException("Unable to attach to transport; please check logs");
        }
        if (log.isDebugEnabled()) {
            log.debug(String.format("Attached %s to %s", publisher, transport));
        }

        return publisher;
    }

    public synchronized Publisher getPublisher() throws JMSException {
        if (publisher == null) {
            publisher = createPublisher();
        }
        return publisher;
    }
}

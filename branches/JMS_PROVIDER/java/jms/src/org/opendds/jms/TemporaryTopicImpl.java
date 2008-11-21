/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.InvalidDestinationException;
import javax.jms.JMSException;
import javax.jms.TemporaryTopic;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import DDS.DomainParticipant;
import DDS.Topic;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TemporaryTopicImpl extends TopicImpl implements TemporaryTopic {
    private static Log log = LogFactory.getLog(TemporaryTopicImpl.class);

    private ConnectionImpl connection;

    public TemporaryTopicImpl(ConnectionImpl connection) {
        super(Strings.randomUuid());
        this.connection = connection;
    }

    public void delete() throws JMSException {
        if (topic != null) {
            if (log.isDebugEnabled()) {
                log.debug(String.format("Deleting " + topic));
            }
            DomainParticipant participant = connection.getParticipant();
            participant.delete_topic(topic);
        }
        connection.deleteTemporaryTopic(this);
    }

    @Override
    public Topic createDDSTopic(ConnectionImpl connection) throws JMSException {
        if (!connection.equals(this.connection)) {
            throw new InvalidDestinationException("TemporaryTopics may not be used with foreign Connections!");
        }
        return super.createDDSTopic(connection);
    }
}

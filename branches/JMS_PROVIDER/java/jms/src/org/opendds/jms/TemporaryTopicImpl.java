/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.InvalidDestinationException;
import javax.jms.JMSException;
import javax.jms.TemporaryTopic;

import DDS.DomainParticipant;
import DDS.Topic;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TemporaryTopicImpl extends TopicImpl implements TemporaryTopic {
    private ConnectionImpl connection;

    public TemporaryTopicImpl(ConnectionImpl connection) {
        super(Strings.randomUuid());
        this.connection = connection;
    }

    public void delete() throws JMSException {
        if (topic != null) {
            DomainParticipant participant = connection.getParticipant();
            participant.delete_topic(topic);
        }
        connection.deleteTemporaryTopic(this);
    }

    @Override
    public Topic toDDSTopic(ConnectionImpl connection) throws JMSException {
        if (!connection.equals(this.connection)) {
            throw new InvalidDestinationException("TemporaryTopics may not be used with foreign Connections!");
        }
        return super.toDDSTopic(connection);
    }
}

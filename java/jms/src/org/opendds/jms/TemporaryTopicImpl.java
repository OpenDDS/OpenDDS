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
import org.opendds.jms.common.util.ContextLog;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TemporaryTopicImpl extends TopicImpl implements TemporaryTopic {
    private ConnectionImpl connection;
    private ContextLog log;

    private transient DDS.Topic topic;

    public TemporaryTopicImpl(ConnectionImpl connection) {
        super(Strings.randomUuid());

        this.connection = connection;
        this.log = connection.getLog();
    }

    @Override
    public Topic createDDSTopic(ConnectionImpl connection) throws JMSException {
        if (!connection.equals(this.connection)) {
            throw new InvalidDestinationException("Invalid Connection!");
        }
        
        if (topic == null) {
            topic = super.createDDSTopic(connection);
        }
        return topic;
    }

    public void delete() throws JMSException {
        if (topic != null) {
            log.debug(String.format("Deleting %s", topic));
            DomainParticipant participant = connection.getParticipant();
            participant.delete_topic(topic);
        }
        connection.deleteTemporaryTopic(this);
    }
}

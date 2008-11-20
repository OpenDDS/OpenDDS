/*
 * $Id$
 */

package org.opendds.jms;

import java.util.EventListener;

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

    public static interface TemporaryTopicListener extends EventListener {
        void onDelete(TemporaryTopicImpl topic);
    }

    private ConnectionImpl connection;
    private TemporaryTopicListener listener;

    public TemporaryTopicImpl(ConnectionImpl connection) {
        this(connection, null);
    }

    public TemporaryTopicImpl(ConnectionImpl connection, TemporaryTopicListener listener) {
        super(Strings.randomUuid());
        this.connection = connection;
        this.listener = listener;
    }

    public void delete() throws JMSException {
        if (topic != null) {
            DomainParticipant participant = connection.getParticipant();
            participant.delete_topic(topic);
        }

        if (listener != null) {
            listener.onDelete(this);
        }
    }

    @Override
    public Topic toDDSTopic(ConnectionImpl connection) throws JMSException {
        if (!connection.equals(this.connection)) {
            throw new InvalidDestinationException("TemporaryTopics may not be used with foreign Connections!");
        }
        return super.toDDSTopic(connection);
    }
}

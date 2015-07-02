/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.InvalidDestinationException;
import javax.jms.JMSException;
import javax.jms.TemporaryTopic;

import DDS.DomainParticipant;
import DDS.Topic;

import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;

/**
 * @author  Steven Stallion
 */
public class TemporaryTopicImpl extends TopicImpl implements TemporaryTopic {
    private ConnectionImpl connection;
    private transient DDS.Topic topic;

    public TemporaryTopicImpl(ConnectionImpl connection) {
        super(Strings.randomUuid());

        assert connection != null;

        this.connection = connection;
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
            Logger logger = connection.getLogger();
            if (logger.isDebugEnabled()) {
                logger.debug("Deleting %s", Strings.asIdentity(this));
            }

            DomainParticipant participant = connection.getParticipant();
            participant.delete_topic(topic);
        }
        connection.deleteTemporaryTopic(this);
    }
}

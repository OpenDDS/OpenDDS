/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.TemporaryTopic;

import DDS.DomainParticipant;
import DDS.TOPIC_QOS_DEFAULT;
import DDS.Topic;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;

import org.opendds.jms.common.lang.Objects;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TemporaryTopicImpl extends TopicImpl implements TemporaryTopic {
    private TemporaryTopicID temporaryTopicID;
    private ConnectionImpl owningConnection;
    DomainParticipant participant;
    private DDS.Topic ddsTopic;

    public TemporaryTopicImpl(TemporaryTopicID temporaryTopicID, ConnectionImpl owningConnection, DomainParticipant participant) {
        super(temporaryTopicID.toString());
        Objects.ensureNotNull(owningConnection);
        Objects.ensureNotNull(participant);

        this.temporaryTopicID = temporaryTopicID;
        this.owningConnection = owningConnection;
        this.participant = participant;
        this.ddsTopic = createDDSTopic();
        owningConnection.addTemporaryTopic(this);
    }

    private DDS.Topic createDDSTopic() {
        String temporaryTopicName = temporaryTopicID.toString();
        MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        typeSupport.register_type(participant, "OpenDDS::JMS::MessagePayload");
        return participant.create_topic(temporaryTopicName, typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
    }

    public String getTopicName() {
        return temporaryTopicID.toString();
    }

    public void delete() throws JMSException {
        owningConnection.removeTemporaryTopic(this);
        participant.delete_topic(ddsTopic);
    }

    public Connection getConnection() {
        return owningConnection;
    }

    public Topic createTopic() {
        // TODO placeholder, role of this method needs clarification
        return ddsTopic;
    }

    @Override
    public String toString() {
        return temporaryTopicID.toString();
    }

    static TemporaryTopicImpl newTemporaryTopicImpl(ConnectionImpl owningConnection, DomainParticipant participant) {
        TemporaryTopicID temporaryTopicID = TemporaryTopicID.createTemporaryTopicID();
        return new TemporaryTopicImpl(temporaryTopicID, owningConnection, participant);
    }
}

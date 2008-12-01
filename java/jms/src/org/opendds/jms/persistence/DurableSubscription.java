/*
 * $Id$
 */

package org.opendds.jms.persistence;

import java.io.Serializable;
import java.util.HashSet;
import java.util.Set;

import javax.jms.JMSException;
import javax.jms.Message;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DurableSubscription implements Serializable {
    private long id;
    private String clientId;
    private String name;

    private Set<String> acknowledgedIds;

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getClientID() {
        return clientId;
    }

    public void setClientID(String clientId) {
        this.clientId = clientId;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public boolean isAcknowledged(Message message) {
        return acknowledgedIds.contains(requireMessageID(message));
    }

    public Set<String> getAcknowledgedIds() {
        if (acknowledgedIds == null) {
            acknowledgedIds = new HashSet<String>();
        }
        return acknowledgedIds;
    }

    public void addAcknowledgedId(String acknowlegedId) {
        getAcknowledgedIds().add(acknowlegedId);
    }

    public void setAcknowledgedIds(Set<String> acknowledgedIds) {
        this.acknowledgedIds = acknowledgedIds;
    }

    public void addAcknowledgedMessage(Message message) {
        addAcknowledgedId(requireMessageID(message));
    }

    //

    private static String requireMessageID(Message message) {
        try {
            String messageId = message.getJMSMessageID();
            if (Strings.isEmpty(messageId)) {
                throw new IllegalArgumentException("Message ID is a required header field!");
            }
            return messageId;

        } catch (JMSException e) {
            throw new IllegalStateException(e);
        }
    }
}

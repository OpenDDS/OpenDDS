/*
 * $Id$
 */

package org.opendds.jms.persistence;

import java.io.Serializable;

import org.opendds.jms.DurableSubscription;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class AcknowledgedMessage implements Serializable {
    private String clientID;
    private String name;
    private String messageID;

    public AcknowledgedMessage() {}

    public AcknowledgedMessage(DurableSubscription subscription, String messageID) {
        assert subscription != null;
        
        this.clientID = subscription.getClientID();
        this.name = subscription.getName();
        this.messageID = messageID;
    }

    public String getClientID() {
        return clientID;
    }

    public void setClientID(String clientID) {
        this.clientID = clientID;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getMessageID() {
        return messageID;
    }

    public void setMessageID(String messageID) {
        this.messageID = messageID;
    }
}

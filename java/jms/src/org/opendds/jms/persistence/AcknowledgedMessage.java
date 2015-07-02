/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.persistence;

import java.io.Serializable;

import org.opendds.jms.DurableSubscription;
import org.opendds.jms.common.lang.Objects;

/**
 * @author  Steven Stallion
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

    @Override
    public int hashCode() {
        return Objects.hashCode(
            clientID,
            name,
            messageID);
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }

        if (!(o instanceof AcknowledgedMessage)) {
            return false;
        }

        AcknowledgedMessage message = (AcknowledgedMessage) o;
        return Objects.equals(clientID, message.clientID)
            && Objects.equals(name, message.name)
            && Objects.equals(messageID, message.messageID);
    }
}

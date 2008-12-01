/*
 * $Id$
 */

package org.opendds.jms.persistence;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DurableSubscription implements Serializable {
    private long id;
    private String clientId;
    private String name;

    private List<String> messages;

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

    public List<String> getMessages() {
        if (messages == null) {
            messages = new ArrayList<String>();
        }
        return messages;
    }

    public void setMessages(List<String> messages) {
        this.messages = messages;
    }
}

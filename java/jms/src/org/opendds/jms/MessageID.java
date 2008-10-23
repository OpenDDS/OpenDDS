package org.opendds.jms;

import java.util.UUID;

public class MessageID {
    private String id;

    private MessageID(String id) {
        this.id = id;
    }

    public static MessageID createMessageID() {
        UUID uuid = UUID.randomUUID();
        return new MessageID(uuid.toString());
    }

    public String toString() {
        return id;
    }

    public static MessageID fromString(String id) {
        return new MessageID(id);
    }
}

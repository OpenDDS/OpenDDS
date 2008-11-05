package org.opendds.jms;

import java.util.UUID;

public class TemporaryTopicID {
    private String id;

    private TemporaryTopicID(String id) {
        this.id = id;
    }

    public static TemporaryTopicID createTemporaryTopicID() {
        UUID uuid = UUID.randomUUID();
        return new TemporaryTopicID(uuid.toString());
    }

    public String toString() {
        return id;
    }

    public static TemporaryTopicID fromString(String id) {
        return new TemporaryTopicID(id);
    }
}
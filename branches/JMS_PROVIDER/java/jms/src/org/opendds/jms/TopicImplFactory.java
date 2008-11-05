package org.opendds.jms;

import javax.jms.Destination;

import DDS.DomainParticipant;

/**
 * Reconstruct a TopicImpl or TemporaryTopicImpl from a String that represents them.  The String
 * representation are from the toString() method of the classes.
 */
public class TopicImplFactory implements Destination {
    public static Destination fromString(String s, ConnectionImpl owningConnection, DomainParticipant participant) {
        if (isTemporaryTopic(s)) {
            return TemporaryTopicImpl.fromTemporaryTopicIDString(toTemporaryTopicIDString(s), owningConnection, participant);
        } else if (isNonTemporaryTopic(s)){
            return TopicImpl.fromTopicName(toTopicName(s));
        }
        throw new IllegalArgumentException("Cannot reconstruct Destination: " + s);
    }

    private static boolean isTemporaryTopic(String s) {
        return s.startsWith("org.opendds.jms.TemporaryTopicImpl");
    }

    private static boolean isNonTemporaryTopic(String s) {
        return s.startsWith("org.opendds.jms.TopicImpl");
    }

    private static String toTemporaryTopicIDString(String s) {
        return extractContent(s);
    }

    private static String toTopicName(String s) {
        return extractContent(s);
    }

    private static String extractContent(String s) {
        int startIndex = s.indexOf('=') + 1;
        int endIndex = s.lastIndexOf(']');
        if (startIndex < 0 || endIndex < startIndex + 1) throw new IllegalArgumentException("Cannot reconstruct Destination: " + s);
        return s.substring(startIndex, endIndex);
    }
}

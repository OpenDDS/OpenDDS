package org.opendds.jms;

import javax.jms.Message;

import static org.opendds.util.EnumComparator.compare;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessageBodyKind;

public class MessageFactory {
    public static Message buildMessageFromPayload(MessagePayload messagePayload, int handle) {
        final MessageBodyKind bodyKind = messagePayload.theBody.discriminator();
        if (compare(bodyKind, MessageBodyKind.STREAM_KIND)) {
            return new StreamMessageImpl(messagePayload, handle);
        } else if (compare(bodyKind, MessageBodyKind.MAP_KIND)) {
            return new MapMessageImpl(messagePayload, handle);
        } else if (compare(bodyKind, MessageBodyKind.TEXT_KIND)) {
            return new TextMessageImpl(messagePayload, handle);
        } else if (compare(bodyKind, MessageBodyKind.OBJECT_KIND)) {
            return new ObjectMessageImpl(messagePayload, handle);
        } else if (compare(bodyKind, MessageBodyKind.BYTES_KIND)) {
            return new BytesMessageImpl(messagePayload, handle);
        }
        throw new IllegalArgumentException("MessagePayload of unkown kind received: " + bodyKind.toString());
    }
}

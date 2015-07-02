/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import OpenDDS.JMS.MessageBodyKind;
import OpenDDS.JMS.MessagePayload;

import static org.opendds.jms.common.EnumComparator.compare;

/**
 * @author  Weiqi Gao
 */
public class ConsumerMessageFactory {
    /**
     * Creates the appropriate Message object from the underlying DDS MessagePayload.  This method is called
     * on the MessageConsumer side as DDS samples are delivered from the DDS subscriber.
     *
     * @param messagePayload The DDS MessagePayload sample
     * @param handle The DDS instance handle of the DDS MessagePayload sample
     * @param sessionImpl The JMS SessionImpl
     */
    public static AbstractMessageImpl buildMessageFromPayload(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        final MessageBodyKind bodyKind = messagePayload.theBody.discriminator();
        if (compare(bodyKind, MessageBodyKind.STREAM_KIND)) {
            return new StreamMessageImpl(messagePayload, handle, sessionImpl);
        } else if (compare(bodyKind, MessageBodyKind.MAP_KIND)) {
            return new MapMessageImpl(messagePayload, handle, sessionImpl);
        } else if (compare(bodyKind, MessageBodyKind.TEXT_KIND)) {
            return new TextMessageImpl(messagePayload, handle, sessionImpl);
        } else if (compare(bodyKind, MessageBodyKind.OBJECT_KIND)) {
            return new ObjectMessageImpl(messagePayload, handle, sessionImpl);
        } else if (compare(bodyKind, MessageBodyKind.BYTES_KIND)) {
            return new BytesMessageImpl(messagePayload, handle, sessionImpl);
        } else {
            throw new IllegalArgumentException("MessagePayload of unkown kind received: " + bodyKind.toString());
        }
    }
}

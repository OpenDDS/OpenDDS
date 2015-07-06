/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.MessageNotReadableException;
import javax.jms.MessageNotWriteableException;

/**
 * Message body state where the message is write only, i.e., writable but not readable.
 * A freshly created or clearBody()-ed BytesMessage and StreamMessage is in this state.
 *
 * @author  Weiqi Gao
 */
public class MessageStateBodyWriteOnly implements MessageState {
    private AbstractMessageImpl message;

    public MessageStateBodyWriteOnly(AbstractMessageImpl message) {
        this.message = message;
    }

    public void checkReadable() throws MessageNotReadableException {
        throw new MessageNotReadableException("The message is in a body write-only state");
    }

    public void checkWritable() throws MessageNotWriteableException {
        // No-op
    }

    public void makeReadable() {
        message.setBodyState(new MessageStateBodyReadOnly(message));
    }

    public void makeWritable() {
        // No-op
    }
}

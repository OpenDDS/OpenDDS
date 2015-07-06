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
 * Message body state where the message is readonly only, i.e., readable but not writable.
 * A freshly received or reset()-ed BytesMessage and StreamMessage is in this state.
 *
 * @author  Weiqi Gao
 */
public class MessageStateBodyReadOnly implements MessageState {
    private AbstractMessageImpl message;

    public MessageStateBodyReadOnly(AbstractMessageImpl message) {
        this.message = message;
    }

    public void checkReadable() throws MessageNotReadableException {
        // No-op
    }

    public void checkWritable() throws MessageNotWriteableException {
        throw new MessageNotWriteableException("The message is in a body read-only state");
    }

    public void makeReadable() {
        // No-op
    }

    public void makeWritable() {
        message.setBodyState(new MessageStateBodyWriteOnly(message));
    }
}

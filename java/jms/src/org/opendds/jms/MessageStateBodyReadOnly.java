package org.opendds.jms;

import javax.jms.MessageNotWriteableException;
import javax.jms.MessageNotReadableException;

/**
 * Message body state where the message is readonly only, i.e., readable but not writable.
 * A freshly received or reset()-ed BytesMessage and StreamMessage is in this state.
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

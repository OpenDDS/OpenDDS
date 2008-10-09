package org.opendds.jms;

import javax.jms.MessageNotWriteableException;

public class MessageStateNonWritable implements MessageState {
    public void ensureWritable() throws MessageNotWriteableException {
        throw new MessageNotWriteableException("The message is in a non-writable state");
    }
}

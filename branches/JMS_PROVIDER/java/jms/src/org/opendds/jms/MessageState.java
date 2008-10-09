package org.opendds.jms;

import javax.jms.MessageNotWriteableException;

public interface MessageState {
    /**
     * @throws MessageNotWriteableException
     */
    void ensureWritable() throws MessageNotWriteableException;
}

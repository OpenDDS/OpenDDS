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
 * @author  Weiqi Gao
 */
public class MessageStatePropertiesNonWritable implements MessageState {
    private AbstractMessageImpl message;

    public MessageStatePropertiesNonWritable(AbstractMessageImpl message) {
        this.message = message;
    }

    public void checkReadable() throws MessageNotReadableException {
        // No-op
    }

    public void checkWritable() throws MessageNotWriteableException {
        throw new MessageNotWriteableException("The message is in a properties non-writable state");
    }

    public void makeReadable() {
        // No-op
    }

    public void makeWritable() {
        message.setPropertiesState(new MessageStateWritable());
    }
}

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.MessageNotReadableException;

/**
 * Message body state where the message is eradable and writable.
 * A freshly created or clearBody()-ed TextMessage, ObjectMessage and MapMessage that is in
 * this state.
 *
 * @author  Weiqi Gao
 */
public class MessageStateWritable implements MessageState {
    public void checkReadable() throws MessageNotReadableException {
        // No-op
    }

    public void checkWritable() {
        // No-op
    }

    public void makeReadable() {
        // No-op
    }

    public void makeWritable() {
        // No-op
    }
}

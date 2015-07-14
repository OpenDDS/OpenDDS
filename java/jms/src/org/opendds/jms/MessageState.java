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
public interface MessageState {
    /**
     *
     * @throws MessageNotReadableException  if the message is not in a readable state
     */
    void checkReadable() throws MessageNotReadableException;
    /**
     * @throws MessageNotWriteableException  if the message is not in a writable state
     */
    void checkWritable() throws MessageNotWriteableException;

    /**
     * Make the message readable again
     */
    void makeReadable();

    /**
     * Make the message writable again
     */
    void makeWritable();
}

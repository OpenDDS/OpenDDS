/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.EventListener;

/**
 * @author  Steven Stallion
 */
public interface ConnectionStateListener extends EventListener {

    void connectionStarted(ConnectionStateEvent event);

    void connectionStopped(ConnectionStateEvent event);
}

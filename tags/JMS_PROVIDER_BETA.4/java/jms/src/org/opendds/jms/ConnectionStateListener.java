/*
 * $Id$
 */

package org.opendds.jms;

import java.util.EventListener;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ConnectionStateListener extends EventListener {

    void connectionStarted(ConnectionStateEvent event);

    void connectionStopped(ConnectionStateEvent event);
}

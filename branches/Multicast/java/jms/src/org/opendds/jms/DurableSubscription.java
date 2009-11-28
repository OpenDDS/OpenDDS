/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.io.Serializable;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DurableSubscription implements Serializable {
    private String clientID;
    private String name;

    public DurableSubscription(String clientID, String name) {
        this.clientID = clientID;
        this.name = name;
    }

    public String getClientID() {
        return clientID;
    }

    public String getName() {
        return name;
    }
}

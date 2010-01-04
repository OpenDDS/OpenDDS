/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans.spi;

import OpenDDS.DCPS.transport.TransportConfiguration.ThreadSynchStrategy;

import org.opendds.jms.common.beans.UnsupportedTypeException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ThreadSynchStrategyType implements Type<ThreadSynchStrategy> {

    public Class[] supportedTypes() {
        return new Class[] { ThreadSynchStrategy.class };
    }

    public ThreadSynchStrategy defaultValue() {
        return ThreadSynchStrategy.PER_CONNECTION_SYNCH;
    }

    public ThreadSynchStrategy valueOf(Object o) {
        assert o != null;

        if (o instanceof Number) {
            return ThreadSynchStrategy.values()[((Number) o).intValue()];

        } else if (o instanceof String) {
            return ThreadSynchStrategy.valueOf(ThreadSynchStrategy.class, (String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}

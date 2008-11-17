/*
 * $Id$
 */

package org.opendds.jms.common.beans.spi;

import OpenDDS.DCPS.transport.TransportConfiguration.ThreadSynchStrategy;

import org.opendds.jms.common.beans.UnsupportedTypeException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ThreadSynchStrategyType implements Type<ThreadSynchStrategy> {

    public Class<ThreadSynchStrategy> getType() {
        return ThreadSynchStrategy.class;
    }

    public ThreadSynchStrategy defaultValue() {
        return ThreadSynchStrategy.PER_CONNECTION_SYNCH;
    }

    public ThreadSynchStrategy valueOf(Object o) {
        if (o instanceof Number) {
            return ThreadSynchStrategy.values()[((Number) o).intValue()];

        } else if (o instanceof String) {
            return ThreadSynchStrategy.valueOf(getType(), (String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}

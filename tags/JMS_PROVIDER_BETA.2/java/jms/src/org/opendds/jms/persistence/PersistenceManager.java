/*
 * $Id$
 */

package org.opendds.jms.persistence;

import java.io.Serializable;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface PersistenceManager extends Serializable {

    DurableSubscriptionStore getDurableSubscriptionStore();
}

/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
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

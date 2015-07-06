/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.persistence;

import java.io.Serializable;

/**
 * @author  Steven Stallion
 */
public interface PersistenceManager extends Serializable {

    DurableSubscriptionStore getDurableSubscriptionStore();
}

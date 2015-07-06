/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import OpenDDS.JMS.ItemKind;
import OpenDDS.JMS.MessageBodyKind;
import OpenDDS.JMS.PropertyValueKind;

/**
 * @author  Weiqi Gao
 */
public class EnumComparator {

    public static boolean compare(MessageBodyKind lhs, MessageBodyKind rhs) {
        return lhs.value() == rhs.value();
    }

    public static boolean compare(PropertyValueKind lhs, PropertyValueKind rhs) {
        return lhs.value() == rhs.value();
    }

    public static boolean compare(ItemKind lhs, ItemKind rhs) {
        return lhs.value() == rhs.value();
    }
}

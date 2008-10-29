package org.opendds.util;

import OpenDDS.JMS.MessageBodyKind;
import OpenDDS.JMS.PropertyValueKind;
import OpenDDS.JMS.ItemKind;

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

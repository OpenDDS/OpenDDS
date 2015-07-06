/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import javax.jms.MessageFormatException;

import OpenDDS.JMS.ItemKind;
import OpenDDS.JMS.StreamItem;

import static org.opendds.jms.common.EnumComparator.compare;

/**
 * @author  Weiqi Gao
 */
public class StreamItemConversion {
    public static boolean convertToBoolean(StreamItem item) throws MessageFormatException {
        if (item == null) return Boolean.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BOOLEAN_KIND)) {
            return item.booleanValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Boolean.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to boolean");
        }
    }


    public static byte convertToByte(StreamItem item) throws MessageFormatException {
        if (item == null) return Byte.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BYTE_KIND)) {
            return item.byteValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Byte.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to byte");
        }
    }

    public static short convertToShort(StreamItem item) throws MessageFormatException {
        if (item == null) return Short.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BYTE_KIND)) {
            return item.byteValue();
        } else if (compare(itemKind, ItemKind.SHORT_KIND)) {
            return item.shortValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Short.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to short");
        }
    }

    public static char convertToChar(StreamItem item) throws MessageFormatException {
        if (item == null) throw new NullPointerException();

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.CHAR_KIND)) {
            return item.charValue();
        } else {
            throw new MessageFormatException("Cannot convert stream item to char");
        }
    }

    public static int convertToInt(StreamItem item) throws MessageFormatException {
        if (item == null) return Integer.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BYTE_KIND)) {
            return item.byteValue();
        } else if (compare(itemKind, ItemKind.SHORT_KIND)) {
            return item.shortValue();
        } else if (compare(itemKind, ItemKind.INT_KIND)) {
            return item.intValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Integer.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to int");
        }
    }

    public static long convertToLong(StreamItem item) throws MessageFormatException {
        if (item == null) return Long.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BYTE_KIND)) {
            return item.byteValue();
        } else if (compare(itemKind, ItemKind.SHORT_KIND)) {
            return item.shortValue();
        } else if (compare(itemKind, ItemKind.INT_KIND)) {
            return item.intValue();
        } else if (compare(itemKind, ItemKind.LONG_KIND)) {
            return item.longValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Long.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to long");
        }
    }

    public static float convertToFloat(StreamItem item) throws MessageFormatException {
        if (item == null) return Float.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.FLOAT_KIND)) {
            return item.floatValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Float.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to float");
        }
    }

    public static double convertToDouble(StreamItem item) throws MessageFormatException {
        if (item == null) return Double.valueOf(null);

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.FLOAT_KIND)) {
            return item.floatValue();
        } else if (compare(itemKind, ItemKind.DOUBLE_KIND)) {
            return item.doubleValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return Double.valueOf(item.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert stream item to double");
        }
    }

    public static String convertToString(StreamItem item) throws MessageFormatException {
        if (item == null) return null;

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BOOLEAN_KIND)) {
            return Boolean.toString(item.booleanValue());
        } else if (compare(itemKind, ItemKind.BYTE_KIND)) {
            return Byte.toString(item.byteValue());
        } else if (compare(itemKind, ItemKind.SHORT_KIND)) {
            return Short.toString(item.shortValue());
        } else if (compare(itemKind, ItemKind.CHAR_KIND)) {
            return Character.toString(item.charValue());
        } else if (compare(itemKind, ItemKind.INT_KIND)) {
            return Integer.toString(item.intValue());
        } else if (compare(itemKind, ItemKind.LONG_KIND)) {
            return Long.toString(item.longValue());
        } else if (compare(itemKind, ItemKind.FLOAT_KIND)) {
            return Float.toString(item.floatValue());
        } else if (compare(itemKind, ItemKind.DOUBLE_KIND)) {
            return Double.toString(item.doubleValue());
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return item.stringValue();
        } else {
            throw new MessageFormatException("Cannot convert stream item to string");
        }
    }

    public static byte[] convertToByteArray(StreamItem item) throws MessageFormatException {
        if (item == null) return null;

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BYTE_ARRAY_KIND)) {
            return item.byteArrayValue();
        } else {
            throw new MessageFormatException("Cannot convert stream item to bytes");
        }
    }

    public static Object convertToObject(StreamItem item) throws MessageFormatException {
        if (item == null) return null;

        final ItemKind itemKind = item.discriminator();
        if (compare(itemKind, ItemKind.BOOLEAN_KIND)) {
            return item.booleanValue();
        } else if (compare(itemKind, ItemKind.BYTE_KIND)) {
            return item.byteValue();
        } else if (compare(itemKind, ItemKind.SHORT_KIND)) {
            return item.shortValue();
        } else if (compare(itemKind, ItemKind.CHAR_KIND)) {
            return item.charValue();
        } else if (compare(itemKind, ItemKind.INT_KIND)) {
            return item.intValue();
        } else if (compare(itemKind, ItemKind.LONG_KIND)) {
            return item.longValue();
        } else if (compare(itemKind, ItemKind.FLOAT_KIND)) {
            return item.floatValue();
        } else if (compare(itemKind, ItemKind.DOUBLE_KIND)) {
            return item.doubleValue();
        } else if (compare(itemKind, ItemKind.STRING_KIND)) {
            return item.stringValue();
        } else if (compare(itemKind, ItemKind.BYTE_ARRAY_KIND)) {
            return item.byteArrayValue();
        } else {
            throw new MessageFormatException("Cannot convert stream item to object");
        }
    }

}

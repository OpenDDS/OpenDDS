/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import javax.jms.MessageFormatException;

import OpenDDS.JMS.PropertyValue;
import OpenDDS.JMS.PropertyValueKind;

import static org.opendds.jms.common.EnumComparator.compare;

/**
 * @author  Weiqi Gao
 */
public class PropertyValueConversion {
    public static boolean convertToBoolean(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Boolean.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BOOLEAN_PROPERTY_KIND)) {
            return propertyValue.booleanValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Boolean.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to boolean");
        }
    }

    public static byte convertToByte(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Byte.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BYTE_PROPERTY_KIND)) {
            return propertyValue.byteValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Byte.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to byte");
        }
    }

    public static short convertToShort(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Short.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BYTE_PROPERTY_KIND)) {
            return propertyValue.byteValue();
        } else if (compare(kind, PropertyValueKind.SHORT_PROPERTY_KIND)) {
            return propertyValue.shortValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Short.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to short");
        }
    }

    public static int convertToInt(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Integer.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BYTE_PROPERTY_KIND)) {
            return propertyValue.byteValue();
        } else if (compare(kind, PropertyValueKind.SHORT_PROPERTY_KIND)) {
            return propertyValue.shortValue();
        } else if (compare(kind, PropertyValueKind.INT_PROPERTY_KIND)) {
            return propertyValue.intValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Integer.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to int");
        }
    }

    public static long convertToLong(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Long.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BYTE_PROPERTY_KIND)) {
            return propertyValue.byteValue();
        } else if (compare(kind, PropertyValueKind.SHORT_PROPERTY_KIND)) {
            return propertyValue.shortValue();
        } else if (compare(kind, PropertyValueKind.INT_PROPERTY_KIND)) {
            return propertyValue.intValue();
        } else if (compare(kind, PropertyValueKind.LONG_PROPERTY_KIND)) {
            return propertyValue.longValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Long.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to long");
        }
    }

    public static float convertToFloat(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Float.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.FLOAT_PROPERTY_KIND)) {
            return propertyValue.floatValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Float.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to float");
        }
    }

    public static double convertToDouble(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return Double.valueOf(null);

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.FLOAT_PROPERTY_KIND)) {
            return propertyValue.floatValue();
        } else if (compare(kind, PropertyValueKind.DOUBLE_PROPERTY_KIND)) {
            return propertyValue.doubleValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return Double.valueOf(propertyValue.stringValue());
        } else {
            throw new MessageFormatException("Cannot convert property to double");
        }
    }

    public static String convertToString(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return null;

        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BOOLEAN_PROPERTY_KIND)) {
            return Boolean.toString(propertyValue.booleanValue());
        } else if (compare(kind, PropertyValueKind.BYTE_PROPERTY_KIND)) {
            return Byte.toString(propertyValue.byteValue());
        } else if (compare(kind, PropertyValueKind.SHORT_PROPERTY_KIND)) {
            return Short.toString(propertyValue.shortValue());
        } else if (compare(kind, PropertyValueKind.INT_PROPERTY_KIND)) {
            return Integer.toString(propertyValue.intValue());
        } else if (compare(kind, PropertyValueKind.LONG_PROPERTY_KIND)) {
            return Long.toString(propertyValue.longValue());
        } else if (compare(kind, PropertyValueKind.FLOAT_PROPERTY_KIND)) {
            return Float.toString(propertyValue.floatValue());
        } else if (compare(kind, PropertyValueKind.DOUBLE_PROPERTY_KIND)) {
            return Double.toString(propertyValue.doubleValue());
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return propertyValue.stringValue();
        } else {
            throw new MessageFormatException("Cannot convert property to string");
        }
    }

    public static Object convertToObject(PropertyValue propertyValue) throws MessageFormatException {
        if (propertyValue == null) return null;
        final PropertyValueKind kind = propertyValue.discriminator();
        if (compare(kind, PropertyValueKind.BOOLEAN_PROPERTY_KIND)) {
            return propertyValue.booleanValue();
        } else if (compare(kind, PropertyValueKind.BYTE_PROPERTY_KIND)) {
            return propertyValue.byteValue();
        } else if (compare(kind, PropertyValueKind.SHORT_PROPERTY_KIND)) {
            return propertyValue.shortValue();
        } else if (compare(kind, PropertyValueKind.INT_PROPERTY_KIND)) {
            return propertyValue.intValue();
        } else if (compare(kind, PropertyValueKind.LONG_PROPERTY_KIND)) {
            return propertyValue.longValue();
        } else if (compare(kind, PropertyValueKind.FLOAT_PROPERTY_KIND)) {
            return propertyValue.floatValue();
        } else if (compare(kind, PropertyValueKind.DOUBLE_PROPERTY_KIND)) {
            return propertyValue.doubleValue();
        } else if (compare(kind, PropertyValueKind.STRING_PROPERTY_KIND)) {
            return propertyValue.stringValue();
        } else {
            throw new MessageFormatException("Cannot convert property to object");
        }
    }
}

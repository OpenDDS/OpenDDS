/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.eclipse.emf.common.util.Enumerator;

/**
 * <!-- begin-user-doc --> A representation of the literals of the
 * enumeration '<em><b>Simple Type</b></em>', and utility methods for
 * working with them. <!-- end-user-doc -->
 * 
 * @see OpenDDS.OpenDDSPackage#getSimpleType()
 * @model
 * @generated
 */
public enum SimpleType implements Enumerator {
    /**
     * The '<em><b>OBoolean</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OBOOLEAN_VALUE
     * @generated
     * @ordered
     */
    OBOOLEAN(0, "OBoolean", "OBoolean"),

    /**
     * The '<em><b>OChar</b></em>' literal object. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @see #OCHAR_VALUE
     * @generated
     * @ordered
     */
    OCHAR(1, "OChar", "OChar"),

    /**
     * The '<em><b>ODouble</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #ODOUBLE_VALUE
     * @generated
     * @ordered
     */
    ODOUBLE(2, "ODouble", "ODouble"),

    /**
     * The '<em><b>OFloat</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OFLOAT_VALUE
     * @generated
     * @ordered
     */
    OFLOAT(3, "OFloat", "OFloat"),

    /**
     * The '<em><b>OLong</b></em>' literal object. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @see #OLONG_VALUE
     * @generated
     * @ordered
     */
    OLONG(4, "OLong", "OLong"),

    /**
     * The '<em><b>OLong Long</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OLONG_LONG_VALUE
     * @generated
     * @ordered
     */
    OLONG_LONG(5, "OLongLong", "OLongLong"),

    /**
     * The '<em><b>OOctet</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OOCTET_VALUE
     * @generated
     * @ordered
     */
    OOCTET(6, "OOctet", "OOctet"),

    /**
     * The '<em><b>OString</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OSTRING_VALUE
     * @generated
     * @ordered
     */
    OSTRING(7, "OString", "OString"),

    /**
     * The '<em><b>OU Long</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OU_LONG_VALUE
     * @generated
     * @ordered
     */
    OU_LONG(8, "OULong", "OULong"),

    /**
     * The '<em><b>OU Long Long</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OU_LONG_LONG_VALUE
     * @generated
     * @ordered
     */
    OU_LONG_LONG(9, "OULongLong", "OULongLong"),

    /**
     * The '<em><b>OU Short</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #OU_SHORT_VALUE
     * @generated
     * @ordered
     */
    OU_SHORT(10, "OUShort", "OUShort");

    /**
     * The '<em><b>OBoolean</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>OBoolean</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OBOOLEAN
     * @model name="OBoolean"
     * @generated
     * @ordered
     */
    public static final int OBOOLEAN_VALUE = 0;

    /**
     * The '<em><b>OChar</b></em>' literal value. <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>OChar</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OCHAR
     * @model name="OChar"
     * @generated
     * @ordered
     */
    public static final int OCHAR_VALUE = 1;

    /**
     * The '<em><b>ODouble</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>ODouble</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #ODOUBLE
     * @model name="ODouble"
     * @generated
     * @ordered
     */
    public static final int ODOUBLE_VALUE = 2;

    /**
     * The '<em><b>OFloat</b></em>' literal value. <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>OFloat</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OFLOAT
     * @model name="OFloat"
     * @generated
     * @ordered
     */
    public static final int OFLOAT_VALUE = 3;

    /**
     * The '<em><b>OLong</b></em>' literal value. <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>OLong</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OLONG
     * @model name="OLong"
     * @generated
     * @ordered
     */
    public static final int OLONG_VALUE = 4;

    /**
     * The '<em><b>OLong Long</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>OLong Long</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OLONG_LONG
     * @model name="OLongLong"
     * @generated
     * @ordered
     */
    public static final int OLONG_LONG_VALUE = 5;

    /**
     * The '<em><b>OOctet</b></em>' literal value. <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>OOctet</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OOCTET
     * @model name="OOctet"
     * @generated
     * @ordered
     */
    public static final int OOCTET_VALUE = 6;

    /**
     * The '<em><b>OString</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>OString</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OSTRING
     * @model name="OString"
     * @generated
     * @ordered
     */
    public static final int OSTRING_VALUE = 7;

    /**
     * The '<em><b>OU Long</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>OU Long</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OU_LONG
     * @model name="OULong"
     * @generated
     * @ordered
     */
    public static final int OU_LONG_VALUE = 8;

    /**
     * The '<em><b>OU Long Long</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>OU Long Long</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OU_LONG_LONG
     * @model name="OULongLong"
     * @generated
     * @ordered
     */
    public static final int OU_LONG_LONG_VALUE = 9;

    /**
     * The '<em><b>OU Short</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>OU Short</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #OU_SHORT
     * @model name="OUShort"
     * @generated
     * @ordered
     */
    public static final int OU_SHORT_VALUE = 10;

    /**
     * An array of all the '<em><b>Simple Type</b></em>' enumerators.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    private static final SimpleType[] VALUES_ARRAY = new SimpleType[] { OBOOLEAN, OCHAR, ODOUBLE, OFLOAT, OLONG,
            OLONG_LONG, OOCTET, OSTRING, OU_LONG, OU_LONG_LONG, OU_SHORT, };

    /**
     * A public read-only list of all the '<em><b>Simple Type</b></em>
     * ' enumerators. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public static final List<SimpleType> VALUES = Collections.unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Simple Type</b></em>' literal with the
     * specified literal value. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    public static SimpleType get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            SimpleType result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Simple Type</b></em>' literal with the
     * specified name. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public static SimpleType getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            SimpleType result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Simple Type</b></em>' literal with the
     * specified integer value. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    public static SimpleType get(int value) {
        switch (value) {
            case OBOOLEAN_VALUE:
                return OBOOLEAN;
            case OCHAR_VALUE:
                return OCHAR;
            case ODOUBLE_VALUE:
                return ODOUBLE;
            case OFLOAT_VALUE:
                return OFLOAT;
            case OLONG_VALUE:
                return OLONG;
            case OLONG_LONG_VALUE:
                return OLONG_LONG;
            case OOCTET_VALUE:
                return OOCTET;
            case OSTRING_VALUE:
                return OSTRING;
            case OU_LONG_VALUE:
                return OU_LONG;
            case OU_LONG_LONG_VALUE:
                return OU_LONG_LONG;
            case OU_SHORT_VALUE:
                return OU_SHORT;
        }
        return null;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    private final int value;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    private final String name;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    private final String literal;

    /**
     * Only this class can construct instances. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @generated
     */
    private SimpleType(int value, String name, String literal) {
        this.value = value;
        this.name = name;
        this.literal = literal;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public int getValue() {
        return value;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public String getName() {
        return name;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public String getLiteral() {
        return literal;
    }

    /**
     * Returns the literal value of the enumerator, which is its
     * string representation. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public String toString() {
        return literal;
    }

} // SimpleType

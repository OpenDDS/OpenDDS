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
 * <!-- begin-user-doc -->
 * A representation of the literals of the enumeration '<em><b>Destination Order Qos Policy Kind</b></em>',
 * and utility methods for working with them.
 * <!-- end-user-doc -->
 * @see OpenDDS.ModelPackage#getDestinationOrderQosPolicyKind()
 * @model
 * @generated
 */
public enum DestinationOrderQosPolicyKind implements Enumerator {
    /**
     * The '<em><b>BY RECEPTION TIMESTAMP</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #BY_RECEPTION_TIMESTAMP_VALUE
     * @generated
     * @ordered
     */
    BY_RECEPTION_TIMESTAMP(0, "BY_RECEPTION_TIMESTAMP", "BY_RECEPTION_TIMESTAMP"),

    /**
     * The '<em><b>BY SOURCE TIMESTAMP</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #BY_SOURCE_TIMESTAMP_VALUE
     * @generated
     * @ordered
     */
    BY_SOURCE_TIMESTAMP(1, "BY_SOURCE_TIMESTAMP", "BY_SOURCE_TIMESTAMP");

    /**
     * The '<em><b>BY RECEPTION TIMESTAMP</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>BY RECEPTION TIMESTAMP</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #BY_RECEPTION_TIMESTAMP
     * @model
     * @generated
     * @ordered
     */
    public static final int BY_RECEPTION_TIMESTAMP_VALUE = 0;

    /**
     * The '<em><b>BY SOURCE TIMESTAMP</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>BY SOURCE TIMESTAMP</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #BY_SOURCE_TIMESTAMP
     * @model
     * @generated
     * @ordered
     */
    public static final int BY_SOURCE_TIMESTAMP_VALUE = 1;

    /**
     * An array of all the '<em><b>Destination Order Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private static final DestinationOrderQosPolicyKind[] VALUES_ARRAY = new DestinationOrderQosPolicyKind[] {
            BY_RECEPTION_TIMESTAMP, BY_SOURCE_TIMESTAMP, };

    /**
     * A public read-only list of all the '<em><b>Destination Order Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static final List<DestinationOrderQosPolicyKind> VALUES = Collections.unmodifiableList(Arrays
            .asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Destination Order Qos Policy Kind</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static DestinationOrderQosPolicyKind get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            DestinationOrderQosPolicyKind result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Destination Order Qos Policy Kind</b></em>' literal with the specified name.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static DestinationOrderQosPolicyKind getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            DestinationOrderQosPolicyKind result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Destination Order Qos Policy Kind</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static DestinationOrderQosPolicyKind get(int value) {
        switch (value) {
            case BY_RECEPTION_TIMESTAMP_VALUE:
                return BY_RECEPTION_TIMESTAMP;
            case BY_SOURCE_TIMESTAMP_VALUE:
                return BY_SOURCE_TIMESTAMP;
        }
        return null;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private final int value;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private final String name;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private final String literal;

    /**
     * Only this class can construct instances.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private DestinationOrderQosPolicyKind(int value, String name, String literal) {
        this.value = value;
        this.name = name;
        this.literal = literal;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public int getValue() {
        return value;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String getName() {
        return name;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String getLiteral() {
        return literal;
    }

    /**
     * Returns the literal value of the enumerator, which is its string representation.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public String toString() {
        return literal;
    }

} //DestinationOrderQosPolicyKind

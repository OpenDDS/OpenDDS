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
 * A representation of the literals of the enumeration '<em><b>Durability Qos Policy Kind</b></em>',
 * and utility methods for working with them.
 * <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSPackage#getDurabilityQosPolicyKind()
 * @model
 * @generated
 */
public enum DurabilityQosPolicyKind implements Enumerator {
    /**
     * The '<em><b>VOLATILE</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #VOLATILE_VALUE
     * @generated
     * @ordered
     */
    VOLATILE(0, "VOLATILE", "VOLATILE"),

    /**
     * The '<em><b>TRANSIENT</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #TRANSIENT_VALUE
     * @generated
     * @ordered
     */
    TRANSIENT(1, "TRANSIENT", "TRANSIENT"),

    /**
     * The '<em><b>TRANSIENT LOCAL</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #TRANSIENT_LOCAL_VALUE
     * @generated
     * @ordered
     */
    TRANSIENT_LOCAL(2, "TRANSIENT_LOCAL", "TRANSIENT_LOCAL"),

    /**
     * The '<em><b>PERSISTENT</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #PERSISTENT_VALUE
     * @generated
     * @ordered
     */
    PERSISTENT(3, "PERSISTENT", "PERSISTENT");

    /**
     * The '<em><b>VOLATILE</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>VOLATILE</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #VOLATILE
     * @model
     * @generated
     * @ordered
     */
    public static final int VOLATILE_VALUE = 0;

    /**
     * The '<em><b>TRANSIENT</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>TRANSIENT</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #TRANSIENT
     * @model
     * @generated
     * @ordered
     */
    public static final int TRANSIENT_VALUE = 1;

    /**
     * The '<em><b>TRANSIENT LOCAL</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>TRANSIENT LOCAL</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #TRANSIENT_LOCAL
     * @model
     * @generated
     * @ordered
     */
    public static final int TRANSIENT_LOCAL_VALUE = 2;

    /**
     * The '<em><b>PERSISTENT</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>PERSISTENT</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #PERSISTENT
     * @model
     * @generated
     * @ordered
     */
    public static final int PERSISTENT_VALUE = 3;

    /**
     * An array of all the '<em><b>Durability Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private static final DurabilityQosPolicyKind[] VALUES_ARRAY = new DurabilityQosPolicyKind[] { VOLATILE, TRANSIENT,
            TRANSIENT_LOCAL, PERSISTENT, };

    /**
     * A public read-only list of all the '<em><b>Durability Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static final List<DurabilityQosPolicyKind> VALUES = Collections
            .unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Durability Qos Policy Kind</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static DurabilityQosPolicyKind get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            DurabilityQosPolicyKind result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Durability Qos Policy Kind</b></em>' literal with the specified name.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static DurabilityQosPolicyKind getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            DurabilityQosPolicyKind result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Durability Qos Policy Kind</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static DurabilityQosPolicyKind get(int value) {
        switch (value) {
            case VOLATILE_VALUE:
                return VOLATILE;
            case TRANSIENT_VALUE:
                return TRANSIENT;
            case TRANSIENT_LOCAL_VALUE:
                return TRANSIENT_LOCAL;
            case PERSISTENT_VALUE:
                return PERSISTENT;
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
    private DurabilityQosPolicyKind(int value, String name, String literal) {
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

} //DurabilityQosPolicyKind

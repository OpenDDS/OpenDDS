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
 * enumeration '<em><b>Ownership Qos Policy Kind</b></em>', and
 * utility methods for working with them. <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSPackage#getOwnershipQosPolicyKind()
 * @model
 * @generated
 */
public enum OwnershipQosPolicyKind implements Enumerator {
    /**
     * The '<em><b>SHARED</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #SHARED_VALUE
     * @generated
     * @ordered
     */
    SHARED(0, "SHARED", "SHARED"),

    /**
     * The '<em><b>EXCLUSIVE</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #EXCLUSIVE_VALUE
     * @generated
     * @ordered
     */
    EXCLUSIVE(1, "EXCLUSIVE", "EXCLUSIVE");

    /**
     * The '<em><b>SHARED</b></em>' literal value.
     * <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>SHARED</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #SHARED
     * @model
     * @generated
     * @ordered
     */
    public static final int SHARED_VALUE = 0;

    /**
     * The '<em><b>EXCLUSIVE</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>EXCLUSIVE</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #EXCLUSIVE
     * @model
     * @generated
     * @ordered
     */
    public static final int EXCLUSIVE_VALUE = 1;

    /**
     * An array of all the '<em><b>Ownership Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    private static final OwnershipQosPolicyKind[] VALUES_ARRAY = new OwnershipQosPolicyKind[] { SHARED, EXCLUSIVE, };

    /**
     * A public read-only list of all the '
     * <em><b>Ownership Qos Policy Kind</b></em>' enumerators. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @generated
     */
    public static final List<OwnershipQosPolicyKind> VALUES = Collections.unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Ownership Qos Policy Kind</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static OwnershipQosPolicyKind get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            OwnershipQosPolicyKind result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Ownership Qos Policy Kind</b></em>' literal with the specified name.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static OwnershipQosPolicyKind getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            OwnershipQosPolicyKind result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Ownership Qos Policy Kind</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static OwnershipQosPolicyKind get(int value) {
        switch (value) {
            case SHARED_VALUE:
                return SHARED;
            case EXCLUSIVE_VALUE:
                return EXCLUSIVE;
        }
        return null;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    private final int value;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    private final String name;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    private final String literal;

    /**
     * Only this class can construct instances.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @generated
     */
    private OwnershipQosPolicyKind(int value, String name, String literal) {
        this.value = value;
        this.name = name;
        this.literal = literal;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public int getValue() {
        return value;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public String getName() {
        return name;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public String getLiteral() {
        return literal;
    }

    /**
     * Returns the literal value of the enumerator, which is its string representation.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    @Override
    public String toString() {
        return literal;
    }

} // OwnershipQosPolicyKind

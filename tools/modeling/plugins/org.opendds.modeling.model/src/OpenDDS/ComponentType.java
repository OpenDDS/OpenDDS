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
 * A representation of the literals of the enumeration '<em><b>Component Type</b></em>',
 * and utility methods for working with them.
 * <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSPackage#getComponentType()
 * @model
 * @generated
 */
public enum ComponentType implements Enumerator {
    /**
     * The '<em><b>EXECUTABLE</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #EXECUTABLE_VALUE
     * @generated
     * @ordered
     */
    EXECUTABLE(0, "EXECUTABLE", "EXECUTABLE"),

    /**
     * The '<em><b>SHARED LIBRARY</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #SHARED_LIBRARY_VALUE
     * @generated
     * @ordered
     */
    SHARED_LIBRARY(1, "SHARED_LIBRARY", "SHARED_LIBRARY"),

    /**
     * The '<em><b>STATIC LIBRARY</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #STATIC_LIBRARY_VALUE
     * @generated
     * @ordered
     */
    STATIC_LIBRARY(2, "STATIC_LIBRARY", "STATIC_LIBRARY");

    /**
     * The '<em><b>EXECUTABLE</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>EXECUTABLE</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #EXECUTABLE
     * @model
     * @generated
     * @ordered
     */
    public static final int EXECUTABLE_VALUE = 0;

    /**
     * The '<em><b>SHARED LIBRARY</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>SHARED LIBRARY</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #SHARED_LIBRARY
     * @model
     * @generated
     * @ordered
     */
    public static final int SHARED_LIBRARY_VALUE = 1;

    /**
     * The '<em><b>STATIC LIBRARY</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>STATIC LIBRARY</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #STATIC_LIBRARY
     * @model
     * @generated
     * @ordered
     */
    public static final int STATIC_LIBRARY_VALUE = 2;

    /**
     * An array of all the '<em><b>Component Type</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private static final ComponentType[] VALUES_ARRAY = new ComponentType[] { EXECUTABLE, SHARED_LIBRARY,
            STATIC_LIBRARY, };

    /**
     * A public read-only list of all the '<em><b>Component Type</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static final List<ComponentType> VALUES = Collections.unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Component Type</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static ComponentType get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            ComponentType result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Component Type</b></em>' literal with the specified name.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static ComponentType getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            ComponentType result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Component Type</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static ComponentType get(int value) {
        switch (value) {
            case EXECUTABLE_VALUE:
                return EXECUTABLE;
            case SHARED_LIBRARY_VALUE:
                return SHARED_LIBRARY;
            case STATIC_LIBRARY_VALUE:
                return STATIC_LIBRARY;
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
    private ComponentType(int value, String name, String literal) {
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

} //ComponentType

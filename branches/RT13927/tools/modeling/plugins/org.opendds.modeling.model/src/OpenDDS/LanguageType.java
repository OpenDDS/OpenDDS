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
 * enumeration '<em><b>Language Type</b></em>', and utility methods
 * for working with them. <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSPackage#getLanguageType()
 * @model
 * @generated
 */
public enum LanguageType implements Enumerator {
    /**
     * The '<em><b>CXX</b></em>' literal object.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @see #CXX_VALUE
     * @generated
     * @ordered
     */
    CXX(0, "CXX", "CXX");

    /**
     * The '<em><b>CXX</b></em>' literal value.
     * <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>CXX</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #CXX
     * @model
     * @generated
     * @ordered
     */
    public static final int CXX_VALUE = 0;

    /**
     * An array of all the '<em><b>Language Type</b></em>' enumerators.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    private static final LanguageType[] VALUES_ARRAY = new LanguageType[] { CXX, };

    /**
     * A public read-only list of all the '<em><b>Language Type</b></em>' enumerators.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @generated
     */
    public static final List<LanguageType> VALUES = Collections.unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Language Type</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static LanguageType get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            LanguageType result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Language Type</b></em>' literal with the specified name.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public static LanguageType getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            LanguageType result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Language Type</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static LanguageType get(int value) {
        switch (value) {
            case CXX_VALUE:
                return CXX;
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
    private LanguageType(int value, String name, String literal) {
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

} // LanguageType

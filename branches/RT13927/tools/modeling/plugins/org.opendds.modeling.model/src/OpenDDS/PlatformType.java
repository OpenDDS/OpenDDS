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
 * enumeration '<em><b>Platform Type</b></em>', and utility methods
 * for working with them. <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSPackage#getPlatformType()
 * @model
 * @generated
 */
public enum PlatformType implements Enumerator {
    /**
     * The '<em><b>MPC CDT</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #MPC_CDT_VALUE
     * @generated
     * @ordered
     */
    MPC_CDT(0, "MPC_CDT", "MPC_CDT"),

    /**
     * The '<em><b>MPC GNUACE</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #MPC_GNUACE_VALUE
     * @generated
     * @ordered
     */
    MPC_GNUACE(1, "MPC_GNUACE", "MPC_GNUACE"),

    /**
     * The '<em><b>MPC NMAKE</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #MPC_NMAKE_VALUE
     * @generated
     * @ordered
     */
    MPC_NMAKE(2, "MPC_NMAKE", "MPC_NMAKE"),

    /**
     * The '<em><b>MPC VC71</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #MPC_VC71_VALUE
     * @generated
     * @ordered
     */
    MPC_VC71(3, "MPC_VC71", "MPC_VC71"),

    /**
     * The '<em><b>MPC VC8</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #MPC_VC8_VALUE
     * @generated
     * @ordered
     */
    MPC_VC8(4, "MPC_VC8", "MPC_VC8"),

    /**
     * The '<em><b>MPC VC9</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @see #MPC_VC9_VALUE
     * @generated
     * @ordered
     */
    MPC_VC9(5, "MPC_VC9", "MPC_VC9");

    /**
     * The '<em><b>MPC CDT</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MPC CDT</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #MPC_CDT
     * @model
     * @generated
     * @ordered
     */
    public static final int MPC_CDT_VALUE = 0;

    /**
     * The '<em><b>MPC GNUACE</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MPC GNUACE</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #MPC_GNUACE
     * @model
     * @generated
     * @ordered
     */
    public static final int MPC_GNUACE_VALUE = 1;

    /**
     * The '<em><b>MPC NMAKE</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MPC NMAKE</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #MPC_NMAKE
     * @model
     * @generated
     * @ordered
     */
    public static final int MPC_NMAKE_VALUE = 2;

    /**
     * The '<em><b>MPC VC71</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MPC VC71</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #MPC_VC71
     * @model
     * @generated
     * @ordered
     */
    public static final int MPC_VC71_VALUE = 3;

    /**
     * The '<em><b>MPC VC8</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MPC VC8</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #MPC_VC8
     * @model
     * @generated
     * @ordered
     */
    public static final int MPC_VC8_VALUE = 4;

    /**
     * The '<em><b>MPC VC9</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MPC VC9</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @see #MPC_VC9
     * @model
     * @generated
     * @ordered
     */
    public static final int MPC_VC9_VALUE = 5;

    /**
     * An array of all the '<em><b>Platform Type</b></em>' enumerators.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    private static final PlatformType[] VALUES_ARRAY = new PlatformType[] { MPC_CDT, MPC_GNUACE, MPC_NMAKE, MPC_VC71,
            MPC_VC8, MPC_VC9, };

    /**
     * A public read-only list of all the '<em><b>Platform Type</b></em>' enumerators.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @generated
     */
    public static final List<PlatformType> VALUES = Collections.unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Platform Type</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static PlatformType get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            PlatformType result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Platform Type</b></em>' literal with the specified name.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public static PlatformType getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            PlatformType result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Platform Type</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public static PlatformType get(int value) {
        switch (value) {
            case MPC_CDT_VALUE:
                return MPC_CDT;
            case MPC_GNUACE_VALUE:
                return MPC_GNUACE;
            case MPC_NMAKE_VALUE:
                return MPC_NMAKE;
            case MPC_VC71_VALUE:
                return MPC_VC71;
            case MPC_VC8_VALUE:
                return MPC_VC8;
            case MPC_VC9_VALUE:
                return MPC_VC9;
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
    private PlatformType(int value, String name, String literal) {
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

} // PlatformType

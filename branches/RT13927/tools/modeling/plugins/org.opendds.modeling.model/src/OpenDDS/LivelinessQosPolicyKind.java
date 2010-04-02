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
 * A representation of the literals of the enumeration '<em><b>Liveliness Qos Policy Kind</b></em>',
 * and utility methods for working with them.
 * <!-- end-user-doc -->
 * @see OpenDDS.ModelPackage#getLivelinessQosPolicyKind()
 * @model
 * @generated
 */
public enum LivelinessQosPolicyKind implements Enumerator {
    /**
     * The '<em><b>AUTOMATIC</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #AUTOMATIC_VALUE
     * @generated
     * @ordered
     */
    AUTOMATIC(0, "AUTOMATIC", "AUTOMATIC"),

    /**
     * The '<em><b>MANUAL BY PARTICIPANT</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #MANUAL_BY_PARTICIPANT_VALUE
     * @generated
     * @ordered
     */
    MANUAL_BY_PARTICIPANT(1, "MANUAL_BY_PARTICIPANT", "MANUAL_BY_PARTICIPANT"),

    /**
     * The '<em><b>MANUAL BY TOPIC</b></em>' literal object.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #MANUAL_BY_TOPIC_VALUE
     * @generated
     * @ordered
     */
    MANUAL_BY_TOPIC(2, "MANUAL_BY_TOPIC", "MANUAL_BY_TOPIC");

    /**
     * The '<em><b>AUTOMATIC</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>AUTOMATIC</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #AUTOMATIC
     * @model
     * @generated
     * @ordered
     */
    public static final int AUTOMATIC_VALUE = 0;

    /**
     * The '<em><b>MANUAL BY PARTICIPANT</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MANUAL BY PARTICIPANT</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #MANUAL_BY_PARTICIPANT
     * @model
     * @generated
     * @ordered
     */
    public static final int MANUAL_BY_PARTICIPANT_VALUE = 1;

    /**
     * The '<em><b>MANUAL BY TOPIC</b></em>' literal value.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>MANUAL BY TOPIC</b></em>' literal object isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @see #MANUAL_BY_TOPIC
     * @model
     * @generated
     * @ordered
     */
    public static final int MANUAL_BY_TOPIC_VALUE = 2;

    /**
     * An array of all the '<em><b>Liveliness Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private static final LivelinessQosPolicyKind[] VALUES_ARRAY = new LivelinessQosPolicyKind[] { AUTOMATIC,
            MANUAL_BY_PARTICIPANT, MANUAL_BY_TOPIC, };

    /**
     * A public read-only list of all the '<em><b>Liveliness Qos Policy Kind</b></em>' enumerators.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static final List<LivelinessQosPolicyKind> VALUES = Collections
            .unmodifiableList(Arrays.asList(VALUES_ARRAY));

    /**
     * Returns the '<em><b>Liveliness Qos Policy Kind</b></em>' literal with the specified literal value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static LivelinessQosPolicyKind get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            LivelinessQosPolicyKind result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Liveliness Qos Policy Kind</b></em>' literal with the specified name.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static LivelinessQosPolicyKind getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            LivelinessQosPolicyKind result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '<em><b>Liveliness Qos Policy Kind</b></em>' literal with the specified integer value.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static LivelinessQosPolicyKind get(int value) {
        switch (value) {
            case AUTOMATIC_VALUE:
                return AUTOMATIC;
            case MANUAL_BY_PARTICIPANT_VALUE:
                return MANUAL_BY_PARTICIPANT;
            case MANUAL_BY_TOPIC_VALUE:
                return MANUAL_BY_TOPIC;
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
    private LivelinessQosPolicyKind(int value, String name, String literal) {
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

} //LivelinessQosPolicyKind

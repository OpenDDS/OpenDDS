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
 * enumeration '
 * <em><b>Presentation Qos Policy Access Scope Kind</b></em>', and
 * utility methods for working with them. <!-- end-user-doc -->
 * 
 * @see OpenDDS.OpenDDSPackage#getPresentationQosPolicyAccessScopeKind()
 * @model
 * @generated
 */
public enum PresentationQosPolicyAccessScopeKind implements Enumerator {
    /**
     * The '<em><b>INSTANCE</b></em>' literal object. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #INSTANCE_VALUE
     * @generated
     * @ordered
     */
    INSTANCE(0, "INSTANCE", "INSTANCE"),

    /**
     * The '<em><b>TOPIC</b></em>' literal object. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @see #TOPIC_VALUE
     * @generated
     * @ordered
     */
    TOPIC(1, "TOPIC", "TOPIC"),

    /**
     * The '<em><b>GROUP</b></em>' literal object. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @see #GROUP_VALUE
     * @generated
     * @ordered
     */
    GROUP(2, "GROUP", "GROUP");

    /**
     * The '<em><b>INSTANCE</b></em>' literal value. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of '<em><b>INSTANCE</b></em>' literal object
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #INSTANCE
     * @model
     * @generated
     * @ordered
     */
    public static final int INSTANCE_VALUE = 0;

    /**
     * The '<em><b>TOPIC</b></em>' literal value. <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>TOPIC</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #TOPIC
     * @model
     * @generated
     * @ordered
     */
    public static final int TOPIC_VALUE = 1;

    /**
     * The '<em><b>GROUP</b></em>' literal value. <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of '<em><b>GROUP</b></em>' literal object isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @see #GROUP
     * @model
     * @generated
     * @ordered
     */
    public static final int GROUP_VALUE = 2;

    /**
     * An array of all the '
     * <em><b>Presentation Qos Policy Access Scope Kind</b></em>'
     * enumerators. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    private static final PresentationQosPolicyAccessScopeKind[] VALUES_ARRAY = new PresentationQosPolicyAccessScopeKind[] {
            INSTANCE, TOPIC, GROUP, };

    /**
     * A public read-only list of all the '
     * <em><b>Presentation Qos Policy Access Scope Kind</b></em>'
     * enumerators. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public static final List<PresentationQosPolicyAccessScopeKind> VALUES = Collections.unmodifiableList(Arrays
            .asList(VALUES_ARRAY));

    /**
     * Returns the '
     * <em><b>Presentation Qos Policy Access Scope Kind</b></em>'
     * literal with the specified literal value. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public static PresentationQosPolicyAccessScopeKind get(String literal) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            PresentationQosPolicyAccessScopeKind result = VALUES_ARRAY[i];
            if (result.toString().equals(literal)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '
     * <em><b>Presentation Qos Policy Access Scope Kind</b></em>'
     * literal with the specified name. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    public static PresentationQosPolicyAccessScopeKind getByName(String name) {
        for (int i = 0; i < VALUES_ARRAY.length; ++i) {
            PresentationQosPolicyAccessScopeKind result = VALUES_ARRAY[i];
            if (result.getName().equals(name)) {
                return result;
            }
        }
        return null;
    }

    /**
     * Returns the '
     * <em><b>Presentation Qos Policy Access Scope Kind</b></em>'
     * literal with the specified integer value. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public static PresentationQosPolicyAccessScopeKind get(int value) {
        switch (value) {
            case INSTANCE_VALUE:
                return INSTANCE;
            case TOPIC_VALUE:
                return TOPIC;
            case GROUP_VALUE:
                return GROUP;
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
    private PresentationQosPolicyAccessScopeKind(int value, String name, String literal) {
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

} // PresentationQosPolicyAccessScopeKind

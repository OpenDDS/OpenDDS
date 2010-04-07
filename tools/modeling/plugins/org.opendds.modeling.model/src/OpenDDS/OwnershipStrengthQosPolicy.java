/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

/**
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Ownership Strength Qos Policy</b></em>'. <!-- end-user-doc
 * -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.OwnershipStrengthQosPolicy#getValue <em>Value
 * </em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getOwnershipStrengthQosPolicy()
 * @model
 * @generated
 */
public interface OwnershipStrengthQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Value</b></em>' attribute.
     * The default value is <code>"0"</code>.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Value</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Value</em>' attribute.
     * @see #setValue(long)
     * @see OpenDDS.OpenDDSPackage#getOwnershipStrengthQosPolicy_Value()
     * @model default="0"
     * @generated
     */
    long getValue();

    /**
     * Sets the value of the '{@link OpenDDS.OwnershipStrengthQosPolicy#getValue <em>Value</em>}' attribute.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @param value the new value of the '<em>Value</em>' attribute.
     * @see #getValue()
     * @generated
     */
    void setValue(long value);

} // OwnershipStrengthQosPolicy

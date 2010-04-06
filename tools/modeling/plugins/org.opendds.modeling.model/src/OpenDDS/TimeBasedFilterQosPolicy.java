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
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Time Based Filter Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.TimeBasedFilterQosPolicy#getMinimum_separation <em>Minimum separation</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getTimeBasedFilterQosPolicy()
 * @model
 * @generated
 */
public interface TimeBasedFilterQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Minimum separation</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Minimum separation</em>' containment reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Minimum separation</em>' containment reference.
     * @see #setMinimum_separation(Period)
     * @see OpenDDS.OpenDDSPackage#getTimeBasedFilterQosPolicy_Minimum_separation()
     * @model containment="true"
     * @generated
     */
    Period getMinimum_separation();

    /**
     * Sets the value of the '{@link OpenDDS.TimeBasedFilterQosPolicy#getMinimum_separation <em>Minimum separation</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Minimum separation</em>' containment reference.
     * @see #getMinimum_separation()
     * @generated
     */
    void setMinimum_separation(Period value);

} // TimeBasedFilterQosPolicy

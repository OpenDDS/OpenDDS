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
 * <em><b>Deadline Qos Policy</b></em>'. <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.DeadlineQosPolicy#getPeriod <em>Period</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getDeadlineQosPolicy()
 * @model
 * @generated
 */
public interface DeadlineQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Period</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Period</em>' containment reference
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Period</em>' containment reference.
     * @see #setPeriod(Period)
     * @see OpenDDS.OpenDDSPackage#getDeadlineQosPolicy_Period()
     * @model containment="true"
     * @generated
     */
    Period getPeriod();

    /**
     * Sets the value of the '{@link OpenDDS.DeadlineQosPolicy#getPeriod <em>Period</em>}' containment reference.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @param value the new value of the '<em>Period</em>' containment reference.
     * @see #getPeriod()
     * @generated
     */
    void setPeriod(Period value);

} // DeadlineQosPolicy

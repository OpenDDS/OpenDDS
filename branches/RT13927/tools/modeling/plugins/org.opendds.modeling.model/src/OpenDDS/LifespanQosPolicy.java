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
 * A representation of the model object '<em><b>Lifespan Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.LifespanQosPolicy#getDuration <em>Duration</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getLifespanQosPolicy()
 * @model
 * @generated
 */
public interface LifespanQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Duration</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Duration</em>' containment reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Duration</em>' containment reference.
     * @see #setDuration(Period)
     * @see OpenDDS.OpenDDSPackage#getLifespanQosPolicy_Duration()
     * @model containment="true"
     * @generated
     */
    Period getDuration();

    /**
     * Sets the value of the '{@link OpenDDS.LifespanQosPolicy#getDuration <em>Duration</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Duration</em>' containment reference.
     * @see #getDuration()
     * @generated
     */
    void setDuration(Period value);

} // LifespanQosPolicy

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
 * A representation of the model object '<em><b>Reliability Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.ReliabilityQosPolicy#getKind <em>Kind</em>}</li>
 *   <li>{@link OpenDDS.ReliabilityQosPolicy#getMax_blocking_time <em>Max blocking time</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getReliabilityQosPolicy()
 * @model
 * @generated
 */
public interface ReliabilityQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Kind</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.ReliabilityQosPolicyKind}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Kind</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.ReliabilityQosPolicyKind
     * @see #setKind(ReliabilityQosPolicyKind)
     * @see OpenDDS.OpenDDSPackage#getReliabilityQosPolicy_Kind()
     * @model
     * @generated
     */
    ReliabilityQosPolicyKind getKind();

    /**
     * Sets the value of the '{@link OpenDDS.ReliabilityQosPolicy#getKind <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.ReliabilityQosPolicyKind
     * @see #getKind()
     * @generated
     */
    void setKind(ReliabilityQosPolicyKind value);

    /**
     * Returns the value of the '<em><b>Max blocking time</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max blocking time</em>' containment reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Max blocking time</em>' containment reference.
     * @see #setMax_blocking_time(Period)
     * @see OpenDDS.OpenDDSPackage#getReliabilityQosPolicy_Max_blocking_time()
     * @model containment="true"
     * @generated
     */
    Period getMax_blocking_time();

    /**
     * Sets the value of the '{@link OpenDDS.ReliabilityQosPolicy#getMax_blocking_time <em>Max blocking time</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Max blocking time</em>' containment reference.
     * @see #getMax_blocking_time()
     * @generated
     */
    void setMax_blocking_time(Period value);

} // ReliabilityQosPolicy

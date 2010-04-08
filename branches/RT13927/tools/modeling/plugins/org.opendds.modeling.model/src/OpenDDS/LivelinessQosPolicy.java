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
 * <em><b>Liveliness Qos Policy</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.LivelinessQosPolicy#getKind <em>Kind</em>}</li>
 * <li>{@link OpenDDS.LivelinessQosPolicy#getLease_duration <em>Lease
 * duration</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getLivelinessQosPolicy()
 * @model
 * @generated
 */
public interface LivelinessQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Kind</b></em>' attribute. The
     * default value is <code>"AUTOMATIC"</code>. The literals are
     * from the enumeration {@link OpenDDS.LivelinessQosPolicyKind}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Kind</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.LivelinessQosPolicyKind
     * @see #setKind(LivelinessQosPolicyKind)
     * @see OpenDDS.OpenDDSPackage#getLivelinessQosPolicy_Kind()
     * @model default="AUTOMATIC"
     * @generated
     */
    LivelinessQosPolicyKind getKind();

    /**
     * Sets the value of the '
     * {@link OpenDDS.LivelinessQosPolicy#getKind <em>Kind</em>}'
     * attribute. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.LivelinessQosPolicyKind
     * @see #getKind()
     * @generated
     */
    void setKind(LivelinessQosPolicyKind value);

    /**
     * Returns the value of the '<em><b>Lease duration</b></em>'
     * containment reference. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Lease duration</em>' containment
     * reference isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Lease duration</em>' containment
     *         reference.
     * @see #setLease_duration(Period)
     * @see OpenDDS.OpenDDSPackage#getLivelinessQosPolicy_Lease_duration()
     * @model containment="true"
     * @generated
     */
    Period getLease_duration();

    /**
     * Sets the value of the '
     * {@link OpenDDS.LivelinessQosPolicy#getLease_duration
     * <em>Lease duration</em>}' containment reference. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Lease duration</em>'
     *            containment reference.
     * @see #getLease_duration()
     * @generated
     */
    void setLease_duration(Period value);

} // LivelinessQosPolicy

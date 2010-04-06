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
 * A representation of the model object '<em><b>History Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.HistoryQosPolicy#getDepth <em>Depth</em>}</li>
 *   <li>{@link OpenDDS.HistoryQosPolicy#getKind <em>Kind</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getHistoryQosPolicy()
 * @model
 * @generated
 */
public interface HistoryQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Depth</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Depth</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Depth</em>' attribute.
     * @see #setDepth(long)
     * @see OpenDDS.OpenDDSPackage#getHistoryQosPolicy_Depth()
     * @model
     * @generated
     */
    long getDepth();

    /**
     * Sets the value of the '{@link OpenDDS.HistoryQosPolicy#getDepth <em>Depth</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Depth</em>' attribute.
     * @see #getDepth()
     * @generated
     */
    void setDepth(long value);

    /**
     * Returns the value of the '<em><b>Kind</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.HistoryQosPolicyKind}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Kind</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.HistoryQosPolicyKind
     * @see #setKind(HistoryQosPolicyKind)
     * @see OpenDDS.OpenDDSPackage#getHistoryQosPolicy_Kind()
     * @model
     * @generated
     */
    HistoryQosPolicyKind getKind();

    /**
     * Sets the value of the '{@link OpenDDS.HistoryQosPolicy#getKind <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.HistoryQosPolicyKind
     * @see #getKind()
     * @generated
     */
    void setKind(HistoryQosPolicyKind value);

} // HistoryQosPolicy

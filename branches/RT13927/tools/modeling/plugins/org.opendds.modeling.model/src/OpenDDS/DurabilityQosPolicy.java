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
 * A representation of the model object '<em><b>Durability Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.DurabilityQosPolicy#getKind <em>Kind</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getDurabilityQosPolicy()
 * @model
 * @generated
 */
public interface DurabilityQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Kind</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.DurabilityQosPolicyKind}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Kind</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.DurabilityQosPolicyKind
     * @see #setKind(DurabilityQosPolicyKind)
     * @see OpenDDS.ModelPackage#getDurabilityQosPolicy_Kind()
     * @model
     * @generated
     */
    DurabilityQosPolicyKind getKind();

    /**
     * Sets the value of the '{@link OpenDDS.DurabilityQosPolicy#getKind <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.DurabilityQosPolicyKind
     * @see #getKind()
     * @generated
     */
    void setKind(DurabilityQosPolicyKind value);

} // DurabilityQosPolicy

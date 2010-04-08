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
 * <em><b>Ownership Qos Policy</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.OwnershipQosPolicy#getKind <em>Kind</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getOwnershipQosPolicy()
 * @model
 * @generated
 */
public interface OwnershipQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Kind</b></em>' attribute. The
     * default value is <code>"SHARED"</code>. The literals are from
     * the enumeration {@link OpenDDS.OwnershipQosPolicyKind}. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Kind</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.OwnershipQosPolicyKind
     * @see #setKind(OwnershipQosPolicyKind)
     * @see OpenDDS.OpenDDSPackage#getOwnershipQosPolicy_Kind()
     * @model default="SHARED"
     * @generated
     */
    OwnershipQosPolicyKind getKind();

    /**
     * Sets the value of the '
     * {@link OpenDDS.OwnershipQosPolicy#getKind <em>Kind</em>}'
     * attribute. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.OwnershipQosPolicyKind
     * @see #getKind()
     * @generated
     */
    void setKind(OwnershipQosPolicyKind value);

} // OwnershipQosPolicy

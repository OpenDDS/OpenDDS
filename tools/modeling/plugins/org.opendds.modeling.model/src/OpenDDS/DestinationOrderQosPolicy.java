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
 * <em><b>Destination Order Qos Policy</b></em>'. <!-- end-user-doc
 * -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.DestinationOrderQosPolicy#getKind <em>Kind</em>}
 * </li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getDestinationOrderQosPolicy()
 * @model
 * @generated
 */
public interface DestinationOrderQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Kind</b></em>' attribute. The
     * default value is <code>"BY_RECEPTION_TIMESTAMP"</code>. The
     * literals are from the enumeration
     * {@link OpenDDS.DestinationOrderQosPolicyKind}. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Kind</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @return the value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.DestinationOrderQosPolicyKind
     * @see #setKind(DestinationOrderQosPolicyKind)
     * @see OpenDDS.OpenDDSPackage#getDestinationOrderQosPolicy_Kind()
     * @model default="BY_RECEPTION_TIMESTAMP"
     * @generated
     */
    DestinationOrderQosPolicyKind getKind();

    /**
     * Sets the value of the '{@link OpenDDS.DestinationOrderQosPolicy#getKind <em>Kind</em>}' attribute.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @param value the new value of the '<em>Kind</em>' attribute.
     * @see OpenDDS.DestinationOrderQosPolicyKind
     * @see #getKind()
     * @generated
     */
    void setKind(DestinationOrderQosPolicyKind value);

} // DestinationOrderQosPolicy

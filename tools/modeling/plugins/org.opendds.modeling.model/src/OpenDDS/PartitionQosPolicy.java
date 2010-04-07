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
 * <em><b>Partition Qos Policy</b></em>'. <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.PartitionQosPolicy#getName <em>Name</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getPartitionQosPolicy()
 * @model
 * @generated
 */
public interface PartitionQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Name</b></em>' attribute. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Name</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @return the value of the '<em>Name</em>' attribute.
     * @see #setName(String)
     * @see OpenDDS.OpenDDSPackage#getPartitionQosPolicy_Name()
     * @model
     * @generated
     */
    String getName();

    /**
     * Sets the value of the '{@link OpenDDS.PartitionQosPolicy#getName <em>Name</em>}' attribute.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @param value the new value of the '<em>Name</em>' attribute.
     * @see #getName()
     * @generated
     */
    void setName(String value);

} // PartitionQosPolicy

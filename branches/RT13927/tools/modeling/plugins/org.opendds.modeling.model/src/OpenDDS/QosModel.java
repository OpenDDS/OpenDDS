/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.util.EList;

/**
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Qos Model</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.QosModel#getQosPolicies <em>Qos Policies</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getQosModel()
 * @model
 * @generated
 */
public interface QosModel extends Model {
    /**
     * Returns the value of the '<em><b>Qos Policies</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.QosPolicy}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Qos Policies</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Qos Policies</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getQosModel_QosPolicies()
     * @model containment="true"
     * @generated
     */
    EList<QosPolicy> getQosPolicies();

} // QosModel

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
 * <em><b>Transport Model</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.TransportModel#getTransports <em>Transports
 * </em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getTransportModel()
 * @model
 * @generated
 */
public interface TransportModel extends Model {
    /**
     * Returns the value of the '<em><b>Transports</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.Transport}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Transports</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Transports</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getTransportModel_Transports()
     * @model containment="true"
     * @generated
     */
    EList<Transport> getTransports();

} // TransportModel

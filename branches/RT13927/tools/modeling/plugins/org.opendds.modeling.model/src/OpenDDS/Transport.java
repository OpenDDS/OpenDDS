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
 * <em><b>Transport</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.Transport#getTransport_id <em>Transport id</em>}
 * </li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getTransport()
 * @model
 * @generated
 */
public interface Transport extends Entity {
    /**
     * Returns the value of the '<em><b>Transport id</b></em>'
     * attribute. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Transport id</em>' attribute isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Transport id</em>' attribute.
     * @see #setTransport_id(long)
     * @see OpenDDS.OpenDDSPackage#getTransport_Transport_id()
     * @model required="true"
     * @generated
     */
    long getTransport_id();

    /**
     * Sets the value of the '
     * {@link OpenDDS.Transport#getTransport_id <em>Transport id</em>}
     * ' attribute. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Transport id</em>'
     *            attribute.
     * @see #getTransport_id()
     * @generated
     */
    void setTransport_id(long value);

} // Transport

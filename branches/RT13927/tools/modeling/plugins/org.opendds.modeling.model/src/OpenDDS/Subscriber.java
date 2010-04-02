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
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Subscriber</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Subscriber#getReaders <em>Readers</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getSubscriber()
 * @model
 * @generated
 */
public interface Subscriber extends PublisherSubscriber {
    /**
     * Returns the value of the '<em><b>Readers</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.DataReader}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Readers</em>' containment reference list isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Readers</em>' containment reference list.
     * @see OpenDDS.ModelPackage#getSubscriber_Readers()
     * @model containment="true" required="true"
     * @generated
     */
    EList<DataReader> getReaders();

} // Subscriber

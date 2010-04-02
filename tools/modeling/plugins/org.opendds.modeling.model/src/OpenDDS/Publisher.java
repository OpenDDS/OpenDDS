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
 * A representation of the model object '<em><b>Publisher</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Publisher#getWriters <em>Writers</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getPublisher()
 * @model
 * @generated
 */
public interface Publisher extends PublisherSubscriber {
    /**
     * Returns the value of the '<em><b>Writers</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.DataWriter}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Writers</em>' containment reference list isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Writers</em>' containment reference list.
     * @see OpenDDS.ModelPackage#getPublisher_Writers()
     * @model containment="true" required="true"
     * @generated
     */
    EList<DataWriter> getWriters();

} // Publisher

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
 * <em><b>Union</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.Union#getCases <em>Cases</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getUnion()
 * @model
 * @generated
 */
public interface Union extends ConstructedTopicType {
    /**
     * Returns the value of the '<em><b>Cases</b></em>' containment
     * reference list. The list contents are of type
     * {@link OpenDDS.Case}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Cases</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Cases</em>' containment reference
     *         list.
     * @see OpenDDS.OpenDDSPackage#getUnion_Cases()
     * @model containment="true" required="true"
     * @generated
     */
    EList<Case> getCases();

} // Union

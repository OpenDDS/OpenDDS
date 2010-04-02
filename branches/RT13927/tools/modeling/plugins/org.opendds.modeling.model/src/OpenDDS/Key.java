/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.ecore.EObject;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Key</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Key#getMember <em>Member</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getKey()
 * @model
 * @generated
 */
public interface Key extends EObject {
    /**
     * Returns the value of the '<em><b>Member</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Member</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Member</em>' reference.
     * @see #setMember(KeyField)
     * @see OpenDDS.ModelPackage#getKey_Member()
     * @model required="true"
     * @generated
     */
    KeyField getMember();

    /**
     * Sets the value of the '{@link OpenDDS.Key#getMember <em>Member</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Member</em>' reference.
     * @see #getMember()
     * @generated
     */
    void setMember(KeyField value);

} // Key

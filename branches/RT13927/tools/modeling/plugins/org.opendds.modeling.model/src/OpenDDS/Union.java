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
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Union</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Union#getSwitch <em>Switch</em>}</li>
 *   <li>{@link OpenDDS.Union#getCases <em>Cases</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getUnion()
 * @model
 * @generated
 */
public interface Union extends ConstructedTopicType {
    /**
     * Returns the value of the '<em><b>Switch</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Switch</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Switch</em>' reference.
     * @see #setSwitch(TopicField)
     * @see OpenDDS.OpenDDSPackage#getUnion_Switch()
     * @model required="true"
     * @generated
     */
    TopicField getSwitch();

    /**
     * Sets the value of the '{@link OpenDDS.Union#getSwitch <em>Switch</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Switch</em>' reference.
     * @see #getSwitch()
     * @generated
     */
    void setSwitch(TopicField value);

    /**
     * Returns the value of the '<em><b>Cases</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Cases</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Cases</em>' reference.
     * @see #setCases(Case)
     * @see OpenDDS.OpenDDSPackage#getUnion_Cases()
     * @model required="true"
     * @generated
     */
    Case getCases();

    /**
     * Sets the value of the '{@link OpenDDS.Union#getCases <em>Cases</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Cases</em>' reference.
     * @see #getCases()
     * @generated
     */
    void setCases(Case value);

} // Union

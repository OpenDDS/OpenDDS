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
 * A representation of the model object '<em><b>Case</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Case#getLabels <em>Labels</em>}</li>
 *   <li>{@link OpenDDS.Case#getType <em>Type</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getCase()
 * @model
 * @generated
 */
public interface Case extends TopicField {
    /**
     * Returns the value of the '<em><b>Labels</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Labels</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Labels</em>' attribute.
     * @see #setLabels(String)
     * @see OpenDDS.OpenDDSPackage#getCase_Labels()
     * @model
     * @generated
     */
    String getLabels();

    /**
     * Sets the value of the '{@link OpenDDS.Case#getLabels <em>Labels</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Labels</em>' attribute.
     * @see #getLabels()
     * @generated
     */
    void setLabels(String value);

    /**
     * Returns the value of the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Type</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Type</em>' reference.
     * @see #setType(TopicField)
     * @see OpenDDS.OpenDDSPackage#getCase_Type()
     * @model required="true"
     * @generated
     */
    TopicField getType();

    /**
     * Sets the value of the '{@link OpenDDS.Case#getType <em>Type</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Type</em>' reference.
     * @see #getType()
     * @generated
     */
    void setType(TopicField value);

} // Case

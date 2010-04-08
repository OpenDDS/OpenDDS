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
 * <em><b>Enum</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.Enum#getLabels <em>Labels</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getEnum()
 * @model
 * @generated
 */
public interface Enum extends TopicField {
    /**
     * Returns the value of the '<em><b>Labels</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Labels</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Labels</em>' attribute.
     * @see #setLabels(String)
     * @see OpenDDS.OpenDDSPackage#getEnum_Labels()
     * @model
     * @generated
     */
    String getLabels();

    /**
     * Sets the value of the '{@link OpenDDS.Enum#getLabels
     * <em>Labels</em>}' attribute. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Labels</em>' attribute.
     * @see #getLabels()
     * @generated
     */
    void setLabels(String value);

} // Enum

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
 * <em><b>Topic Description</b></em>'. <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.TopicDescription#getType <em>Type</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getTopicDescription()
 * @model abstract="true"
 * @generated
 */
public interface TopicDescription extends DomainEntity, ModelEntity {
    /**
     * Returns the value of the '<em><b>Type</b></em>' reference. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Type</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     *
     * @return the value of the '<em>Type</em>' reference.
     * @see #setType(TopicField)
     * @see OpenDDS.OpenDDSPackage#getTopicDescription_Type()
     * @model required="true"
     * @generated
     */
    TopicField getType();

    /**
     * Sets the value of the '{@link OpenDDS.TopicDescription#getType <em>Type</em>}' reference.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @param value the new value of the '<em>Type</em>' reference.
     * @see #getType()
     * @generated
     */
    void setType(TopicField value);

} // TopicDescription

/**
 * <copyright>
 * </copyright>
 *
 * $Id$
 */
package org.opendds.modeling.model.topics;

import org.opendds.modeling.model.core.Entity;

import org.opendds.modeling.model.types.Struct;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Topic Description</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.topics.TopicDescription#getDatatype <em>Datatype</em>}</li>
 * </ul>
 * </p>
 *
 * @see org.opendds.modeling.model.topics.TopicsPackage#getTopicDescription()
 * @model abstract="true"
 * @generated
 */
public interface TopicDescription extends Entity {
	/**
	 * Returns the value of the '<em><b>Datatype</b></em>' reference.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Datatype</em>' reference isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Datatype</em>' reference.
	 * @see #setDatatype(Struct)
	 * @see org.opendds.modeling.model.topics.TopicsPackage#getTopicDescription_Datatype()
	 * @model required="true"
	 * @generated
	 */
	Struct getDatatype();

	/**
	 * Sets the value of the '{@link org.opendds.modeling.model.topics.TopicDescription#getDatatype <em>Datatype</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @param value the new value of the '<em>Datatype</em>' reference.
	 * @see #getDatatype()
	 * @generated
	 */
	void setDatatype(Struct value);

	/**
	 * Needed because datatype is a non-containment reference but will be
	 * the domain element behind a sub-figure of the Topic figure and GMF
	 * assumes the element behind a sub-figure is owned by the element behind
	 * the main figure.
	 * @generated NOT 
	 */
	public org.eclipse.emf.common.util.EList<org.opendds.modeling.model.types.Type> getTypes();

} // TopicDescription

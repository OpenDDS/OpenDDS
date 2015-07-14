/**
 * <copyright>
 * </copyright>
 *
 */
package org.opendds.modeling.model.topics;

import org.opendds.modeling.model.core.Entity;

import org.opendds.modeling.model.types.Struct;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Topic Description</b></em>'.
 * <!-- end-user-doc -->
 *
 *
 * @see org.opendds.modeling.model.topics.TopicsPackage#getTopicDescription()
 * @model abstract="true"
 * @generated
 */
public interface TopicDescription extends Entity {
	/**
	 * Needed because datatype is a non-containment reference but will be
	 * the domain element behind a sub-figure of the Topic figure and GMF
	 * assumes the element behind a sub-figure is owned by the element behind
	 * the main figure.
	 * @generated NOT 
	 */
	public org.eclipse.emf.common.util.EList<org.opendds.modeling.model.types.Type> getTypes();

} // TopicDescription

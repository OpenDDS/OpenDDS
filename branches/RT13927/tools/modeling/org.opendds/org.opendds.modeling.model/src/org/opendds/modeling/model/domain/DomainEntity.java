/**
 * <copyright>
 * </copyright>
 *
 * $Id$
 */
package org.opendds.modeling.model.domain;

import org.eclipse.emf.common.util.EList;

import org.opendds.modeling.model.core.Entity;
import org.opendds.modeling.model.qos.QosPolicy;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Entity</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.domain.DomainEntity#getQosPolicy <em>Qos Policy</em>}</li>
 * </ul>
 * </p>
 *
 * @see org.opendds.modeling.model.domain.DomainPackage#getDomainEntity()
 * @model
 * @generated
 */
public interface DomainEntity extends Entity {
	/**
	 * Returns the value of the '<em><b>Qos Policy</b></em>' containment reference list.
	 * The list contents are of type {@link org.opendds.modeling.model.domain.QosProperty}.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Qos Policy</em>' containment reference list isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Qos Policy</em>' containment reference list.
	 * @see org.opendds.modeling.model.domain.DomainPackage#getDomainEntity_QosPolicy()
	 * @model containment="true" required="true"
	 * @generated
	 */
	EList<QosProperty> getQosPolicy();

	/**
	 * Needed because the DomainParticipant does not own its policies and GMF assumes that domain elements in figure
	 * compartments are owned by the domain element that is associated with the parent figure.
	 * @generated NOT
	 */
	EList<QosPolicy> getPolicies();

} // DomainEntity

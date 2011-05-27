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
 *
 * @see org.opendds.modeling.model.domain.DomainPackage#getDomainEntity()
 * @model
 * @generated
 */
public interface DomainEntity extends Entity {
	/**
	 * Needed because the DomainParticipant does not own its policies and GMF assumes that domain elements in figure
	 * compartments are owned by the domain element that is associated with the parent figure.
	 * @generated NOT
	 */
	EList<QosPolicy> getPolicies();

} // DomainEntity

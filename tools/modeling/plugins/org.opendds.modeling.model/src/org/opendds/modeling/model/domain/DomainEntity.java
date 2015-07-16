/**
 * <copyright>
 * </copyright>
 *
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
 *   <li>{@link org.opendds.modeling.model.domain.DomainEntity#getTransportConfig <em>Transport Config</em>}</li>
 * </ul>
 * </p>
 *
 * @see org.opendds.modeling.model.domain.DomainPackage#getDomainEntity()
 * @model
 * @generated
 */
public interface DomainEntity extends Entity {
	/**
	 * Returns the value of the '<em><b>Transport Config</b></em>' attribute.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Transport Config</em>' attribute isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Transport Config</em>' attribute.
	 * @see #setTransportConfig(String)
	 * @see org.opendds.modeling.model.domain.DomainPackage#getDomainEntity_TransportConfig()
	 * @model annotation="GenModel documentation='Indicate the identifier of the transport configuration to use. Note that it is currently not intended for Topics to have this attribute used. '"
	 * @generated
	 */
	String getTransportConfig();

	/**
	 * Sets the value of the '{@link org.opendds.modeling.model.domain.DomainEntity#getTransportConfig <em>Transport Config</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @param value the new value of the '<em>Transport Config</em>' attribute.
	 * @see #getTransportConfig()
	 * @generated
	 */
	void setTransportConfig(String value);

	/**
	 * Needed because the DomainParticipant does not own its policies and GMF assumes that domain elements in figure
	 * compartments are owned by the domain element that is associated with the parent figure.
	 * @generated NOT
	 */
	EList<QosPolicy> getPolicies();

} // DomainEntity

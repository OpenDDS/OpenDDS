/**
 * <copyright>
 * </copyright>
 *
 */
package org.opendds.modeling.model.dcps;

import org.eclipse.emf.common.util.EList;

import org.opendds.modeling.model.domain.DomainEntity;

import org.opendds.modeling.model.qos.EntityFactoryQosPolicy;
import org.opendds.modeling.model.qos.QosPolicy;
import org.opendds.modeling.model.qos.UserDataQosPolicy;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Domain Participant</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.dcps.DomainParticipant#getDomain <em>Domain</em>}</li>
 *   <li>{@link org.opendds.modeling.model.dcps.DomainParticipant#getPublishers <em>Publishers</em>}</li>
 *   <li>{@link org.opendds.modeling.model.dcps.DomainParticipant#getSubscribers <em>Subscribers</em>}</li>
 *   <li>{@link org.opendds.modeling.model.dcps.DomainParticipant#getEntity_factory <em>Entity factory</em>}</li>
 *   <li>{@link org.opendds.modeling.model.dcps.DomainParticipant#getUser_data <em>User data</em>}</li>
 * </ul>
 * </p>
 *
 * @see org.opendds.modeling.model.dcps.DCPSPackage#getDomainParticipant()
 * @model
 * @generated
 */
public interface DomainParticipant extends DomainEntity {
	/**
	 * Returns the value of the '<em><b>Domain</b></em>' reference.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Domain</em>' reference isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Domain</em>' reference.
	 * @see #setDomain(Domain)
	 * @see org.opendds.modeling.model.dcps.DCPSPackage#getDomainParticipant_Domain()
	 * @model required="true"
	 * @generated
	 */
	Domain getDomain();

	/**
	 * Sets the value of the '{@link org.opendds.modeling.model.dcps.DomainParticipant#getDomain <em>Domain</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @param value the new value of the '<em>Domain</em>' reference.
	 * @see #getDomain()
	 * @generated
	 */
	void setDomain(Domain value);

	/**
	 * Returns the value of the '<em><b>Publishers</b></em>' containment reference list.
	 * The list contents are of type {@link org.opendds.modeling.model.dcps.Publisher}.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Publishers</em>' containment reference list isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Publishers</em>' containment reference list.
	 * @see org.opendds.modeling.model.dcps.DCPSPackage#getDomainParticipant_Publishers()
	 * @model containment="true"
	 * @generated
	 */
	EList<Publisher> getPublishers();

	/**
	 * Returns the value of the '<em><b>Subscribers</b></em>' containment reference list.
	 * The list contents are of type {@link org.opendds.modeling.model.dcps.Subscriber}.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Subscribers</em>' containment reference list isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Subscribers</em>' containment reference list.
	 * @see org.opendds.modeling.model.dcps.DCPSPackage#getDomainParticipant_Subscribers()
	 * @model containment="true"
	 * @generated
	 */
	EList<Subscriber> getSubscribers();

	/**
	 * Returns the value of the '<em><b>Entity factory</b></em>' reference.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>Entity factory</em>' containment reference isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>Entity factory</em>' reference.
	 * @see #setEntity_factory(EntityFactoryQosPolicy)
	 * @see org.opendds.modeling.model.dcps.DCPSPackage#getDomainParticipant_Entity_factory()
	 * @model
	 * @generated
	 */
	EntityFactoryQosPolicy getEntity_factory();

	/**
	 * Sets the value of the '{@link org.opendds.modeling.model.dcps.DomainParticipant#getEntity_factory <em>Entity factory</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @param value the new value of the '<em>Entity factory</em>' reference.
	 * @see #getEntity_factory()
	 * @generated
	 */
	void setEntity_factory(EntityFactoryQosPolicy value);

	/**
	 * Returns the value of the '<em><b>User data</b></em>' reference.
	 * <!-- begin-user-doc -->
	 * <p>
	 * If the meaning of the '<em>User data</em>' containment reference isn't clear,
	 * there really should be more of a description here...
	 * </p>
	 * <!-- end-user-doc -->
	 * @return the value of the '<em>User data</em>' reference.
	 * @see #setUser_data(UserDataQosPolicy)
	 * @see org.opendds.modeling.model.dcps.DCPSPackage#getDomainParticipant_User_data()
	 * @model
	 * @generated
	 */
	UserDataQosPolicy getUser_data();

	/**
	 * Sets the value of the '{@link org.opendds.modeling.model.dcps.DomainParticipant#getUser_data <em>User data</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @param value the new value of the '<em>User data</em>' reference.
	 * @see #getUser_data()
	 * @generated
	 */
	void setUser_data(UserDataQosPolicy value);

	/**
	 * Needed because the DomainParticipant does not own its policies and GMF assumes that domain elements in figure
	 * compartments are owned by the domain element that is associated with the parent figure.
	 * @generated NOT
	 */
	EList<QosPolicy> getPolicies();

} // DomainParticipant

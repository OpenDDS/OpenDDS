/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.util.EList;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Domain Participant</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.DomainParticipant#getSubscribers <em>Subscribers</em>}</li>
 *   <li>{@link OpenDDS.DomainParticipant#getPublishers <em>Publishers</em>}</li>
 *   <li>{@link OpenDDS.DomainParticipant#getEntity_factory <em>Entity factory</em>}</li>
 *   <li>{@link OpenDDS.DomainParticipant#getUser_data <em>User data</em>}</li>
 *   <li>{@link OpenDDS.DomainParticipant#getDomain <em>Domain</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getDomainParticipant()
 * @model
 * @generated
 */
public interface DomainParticipant extends DomainEntity {
    /**
     * Returns the value of the '<em><b>Subscribers</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.Subscriber}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Subscribers</em>' containment reference list isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Subscribers</em>' containment reference list.
     * @see OpenDDS.OpenDDSPackage#getDomainParticipant_Subscribers()
     * @model containment="true"
     * @generated
     */
    EList<Subscriber> getSubscribers();

    /**
     * Returns the value of the '<em><b>Publishers</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.Publisher}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Publishers</em>' containment reference list isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Publishers</em>' containment reference list.
     * @see OpenDDS.OpenDDSPackage#getDomainParticipant_Publishers()
     * @model containment="true"
     * @generated
     */
    EList<Publisher> getPublishers();

    /**
     * Returns the value of the '<em><b>Entity factory</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Entity factory</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Entity factory</em>' reference.
     * @see #setEntity_factory(EntityFactoryQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getDomainParticipant_Entity_factory()
     * @model
     * @generated
     */
    EntityFactoryQosPolicy getEntity_factory();

    /**
     * Sets the value of the '{@link OpenDDS.DomainParticipant#getEntity_factory <em>Entity factory</em>}' reference.
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
     * If the meaning of the '<em>User data</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>User data</em>' reference.
     * @see #setUser_data(UserDataQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getDomainParticipant_User_data()
     * @model
     * @generated
     */
    UserDataQosPolicy getUser_data();

    /**
     * Sets the value of the '{@link OpenDDS.DomainParticipant#getUser_data <em>User data</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>User data</em>' reference.
     * @see #getUser_data()
     * @generated
     */
    void setUser_data(UserDataQosPolicy value);

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
     * @see OpenDDS.OpenDDSPackage#getDomainParticipant_Domain()
     * @model required="true"
     * @generated
     */
    Domain getDomain();

    /**
     * Sets the value of the '{@link OpenDDS.DomainParticipant#getDomain <em>Domain</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Domain</em>' reference.
     * @see #getDomain()
     * @generated
     */
    void setDomain(Domain value);

} // DomainParticipant

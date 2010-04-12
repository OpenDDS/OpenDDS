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
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Domain Model</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.DomainModel#getDomains <em>Domains</em>}</li>
 * <li>{@link OpenDDS.DomainModel#getParticipants <em>Participants
 * </em>}</li>
 * <li>{@link OpenDDS.DomainModel#getTopics <em>Topics</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getDomainModel()
 * @model
 * @generated
 */
public interface DomainModel extends Model {
    /**
     * Returns the value of the '<em><b>Domains</b></em>' containment
     * reference list. The list contents are of type
     * {@link OpenDDS.Domain}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Domains</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Domains</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getDomainModel_Domains()
     * @model containment="true"
     * @generated
     */
    EList<Domain> getDomains();

    /**
     * Returns the value of the '<em><b>Participants</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.DomainParticipant}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Participants</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Participants</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getDomainModel_Participants()
     * @model containment="true"
     * @generated
     */
    EList<DomainParticipant> getParticipants();

    /**
     * Returns the value of the '<em><b>Topics</b></em>' containment
     * reference list. The list contents are of type
     * {@link OpenDDS.TopicDescription}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Topics</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Topics</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getDomainModel_Topics()
     * @model containment="true"
     * @generated
     */
    EList<TopicDescription> getTopics();

} // DomainModel

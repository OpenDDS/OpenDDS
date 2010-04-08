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
 * <em><b>Model</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.Model#getApplications <em>Applications</em>}</li>
 * <li>{@link OpenDDS.Model#getDomains <em>Domains</em>}</li>
 * <li>{@link OpenDDS.Model#getParticipants <em>Participants</em>}</li>
 * <li>{@link OpenDDS.Model#getTopics <em>Topics</em>}</li>
 * <li>{@link OpenDDS.Model#getTopicTypes <em>Topic Types</em>}</li>
 * <li>{@link OpenDDS.Model#getQosPolicies <em>Qos Policies</em>}</li>
 * <li>{@link OpenDDS.Model#getTransports <em>Transports</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getModel()
 * @model
 * @generated
 */
public interface Model extends Entity {
    /**
     * Returns the value of the '<em><b>Applications</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.ApplicationTarget}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Applications</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Applications</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getModel_Applications()
     * @model containment="true"
     * @generated
     */
    EList<ApplicationTarget> getApplications();

    /**
     * Returns the value of the '<em><b>Domains</b></em>' containment
     * reference list. The list contents are of type
     * {@link OpenDDS.Domain}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Domains</em>' reference list isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Domains</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getModel_Domains()
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
     * @see OpenDDS.OpenDDSPackage#getModel_Participants()
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
     * @see OpenDDS.OpenDDSPackage#getModel_Topics()
     * @model containment="true"
     * @generated
     */
    EList<TopicDescription> getTopics();

    /**
     * Returns the value of the '<em><b>Topic Types</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.TopicStruct}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Topic Types</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Topic Types</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getModel_TopicTypes()
     * @model containment="true"
     * @generated
     */
    EList<TopicStruct> getTopicTypes();

    /**
     * Returns the value of the '<em><b>Qos Policies</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.QosPolicy}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Qos Policies</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Qos Policies</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getModel_QosPolicies()
     * @model containment="true"
     * @generated
     */
    EList<QosPolicy> getQosPolicies();

    /**
     * Returns the value of the '<em><b>Transports</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.Transport}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Transports</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Transports</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getModel_Transports()
     * @model containment="true"
     * @generated
     */
    EList<Transport> getTransports();

} // Model

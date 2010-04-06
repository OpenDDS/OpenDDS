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
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Publisher Subscriber</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.PublisherSubscriber#getEntity_factory <em>Entity factory</em>}</li>
 *   <li>{@link OpenDDS.PublisherSubscriber#getPresentation <em>Presentation</em>}</li>
 *   <li>{@link OpenDDS.PublisherSubscriber#getGroup_data <em>Group data</em>}</li>
 *   <li>{@link OpenDDS.PublisherSubscriber#getPartition <em>Partition</em>}</li>
 *   <li>{@link OpenDDS.PublisherSubscriber#getTransport <em>Transport</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getPublisherSubscriber()
 * @model abstract="true"
 * @generated
 */
public interface PublisherSubscriber extends DomainEntity {
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
     * @see OpenDDS.OpenDDSPackage#getPublisherSubscriber_Entity_factory()
     * @model
     * @generated
     */
    EntityFactoryQosPolicy getEntity_factory();

    /**
     * Sets the value of the '{@link OpenDDS.PublisherSubscriber#getEntity_factory <em>Entity factory</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Entity factory</em>' reference.
     * @see #getEntity_factory()
     * @generated
     */
    void setEntity_factory(EntityFactoryQosPolicy value);

    /**
     * Returns the value of the '<em><b>Presentation</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Presentation</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Presentation</em>' reference.
     * @see #setPresentation(PresentationQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getPublisherSubscriber_Presentation()
     * @model
     * @generated
     */
    PresentationQosPolicy getPresentation();

    /**
     * Sets the value of the '{@link OpenDDS.PublisherSubscriber#getPresentation <em>Presentation</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Presentation</em>' reference.
     * @see #getPresentation()
     * @generated
     */
    void setPresentation(PresentationQosPolicy value);

    /**
     * Returns the value of the '<em><b>Group data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Group data</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Group data</em>' reference.
     * @see #setGroup_data(GroupDataQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getPublisherSubscriber_Group_data()
     * @model
     * @generated
     */
    GroupDataQosPolicy getGroup_data();

    /**
     * Sets the value of the '{@link OpenDDS.PublisherSubscriber#getGroup_data <em>Group data</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Group data</em>' reference.
     * @see #getGroup_data()
     * @generated
     */
    void setGroup_data(GroupDataQosPolicy value);

    /**
     * Returns the value of the '<em><b>Partition</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Partition</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Partition</em>' reference.
     * @see #setPartition(PartitionQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getPublisherSubscriber_Partition()
     * @model
     * @generated
     */
    PartitionQosPolicy getPartition();

    /**
     * Sets the value of the '{@link OpenDDS.PublisherSubscriber#getPartition <em>Partition</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Partition</em>' reference.
     * @see #getPartition()
     * @generated
     */
    void setPartition(PartitionQosPolicy value);

    /**
     * Returns the value of the '<em><b>Transport</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Transport</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Transport</em>' reference.
     * @see #setTransport(Transport)
     * @see OpenDDS.OpenDDSPackage#getPublisherSubscriber_Transport()
     * @model required="true"
     * @generated
     */
    Transport getTransport();

    /**
     * Sets the value of the '{@link OpenDDS.PublisherSubscriber#getTransport <em>Transport</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Transport</em>' reference.
     * @see #getTransport()
     * @generated
     */
    void setTransport(Transport value);

} // PublisherSubscriber

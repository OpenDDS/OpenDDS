/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;
import org.eclipse.emf.ecore.impl.ENotificationImpl;

/**
 * <!-- begin-user-doc --> An implementation of the model object '
 * <em><b>Publisher Subscriber</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.PublisherSubscriberImpl#getEntity_factory <em>
 * Entity factory</em>}</li>
 * <li>{@link OpenDDS.PublisherSubscriberImpl#getPresentation <em>
 * Presentation</em>}</li>
 * <li>{@link OpenDDS.PublisherSubscriberImpl#getGroup_data <em>Group
 * data</em>}</li>
 * <li>{@link OpenDDS.PublisherSubscriberImpl#getPartition <em>
 * Partition</em>}</li>
 * <li>{@link OpenDDS.PublisherSubscriberImpl#getTransport <em>
 * Transport</em>}</li>
 * </ul>
 * </p>
 * 
 * @generated
 */
public abstract class PublisherSubscriberImpl extends DomainEntityImpl implements PublisherSubscriber {
    /**
     * The cached value of the '{@link #getEntity_factory()
     * <em>Entity factory</em>}' reference. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @see #getEntity_factory()
     * @generated
     * @ordered
     */
    protected EntityFactoryQosPolicy entity_factory;

    /**
     * The cached value of the '{@link #getPresentation()
     * <em>Presentation</em>}' reference. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getPresentation()
     * @generated
     * @ordered
     */
    protected PresentationQosPolicy presentation;

    /**
     * The cached value of the '{@link #getGroup_data()
     * <em>Group data</em>}' reference. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getGroup_data()
     * @generated
     * @ordered
     */
    protected GroupDataQosPolicy group_data;

    /**
     * The cached value of the '{@link #getPartition()
     * <em>Partition</em>}' reference. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getPartition()
     * @generated
     * @ordered
     */
    protected PartitionQosPolicy partition;

    /**
     * The cached value of the '{@link #getTransport()
     * <em>Transport</em>}' reference. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getTransport()
     * @generated
     * @ordered
     */
    protected Transport transport;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected PublisherSubscriberImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.PUBLISHER_SUBSCRIBER;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EntityFactoryQosPolicy getEntity_factory() {
        if (entity_factory != null && entity_factory.eIsProxy()) {
            InternalEObject oldEntity_factory = (InternalEObject) entity_factory;
            entity_factory = (EntityFactoryQosPolicy) eResolveProxy(oldEntity_factory);
            if (entity_factory != oldEntity_factory) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY, oldEntity_factory, entity_factory));
                }
            }
        }
        return entity_factory;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EntityFactoryQosPolicy basicGetEntity_factory() {
        return entity_factory;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setEntity_factory(EntityFactoryQosPolicy newEntity_factory) {
        EntityFactoryQosPolicy oldEntity_factory = entity_factory;
        entity_factory = newEntity_factory;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY,
                    oldEntity_factory, entity_factory));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public PresentationQosPolicy getPresentation() {
        if (presentation != null && presentation.eIsProxy()) {
            InternalEObject oldPresentation = (InternalEObject) presentation;
            presentation = (PresentationQosPolicy) eResolveProxy(oldPresentation);
            if (presentation != oldPresentation) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION, oldPresentation, presentation));
                }
            }
        }
        return presentation;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public PresentationQosPolicy basicGetPresentation() {
        return presentation;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setPresentation(PresentationQosPolicy newPresentation) {
        PresentationQosPolicy oldPresentation = presentation;
        presentation = newPresentation;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION,
                    oldPresentation, presentation));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public GroupDataQosPolicy getGroup_data() {
        if (group_data != null && group_data.eIsProxy()) {
            InternalEObject oldGroup_data = (InternalEObject) group_data;
            group_data = (GroupDataQosPolicy) eResolveProxy(oldGroup_data);
            if (group_data != oldGroup_data) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA, oldGroup_data, group_data));
                }
            }
        }
        return group_data;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public GroupDataQosPolicy basicGetGroup_data() {
        return group_data;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setGroup_data(GroupDataQosPolicy newGroup_data) {
        GroupDataQosPolicy oldGroup_data = group_data;
        group_data = newGroup_data;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA,
                    oldGroup_data, group_data));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public PartitionQosPolicy getPartition() {
        if (partition != null && partition.eIsProxy()) {
            InternalEObject oldPartition = (InternalEObject) partition;
            partition = (PartitionQosPolicy) eResolveProxy(oldPartition);
            if (partition != oldPartition) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.PUBLISHER_SUBSCRIBER__PARTITION, oldPartition, partition));
                }
            }
        }
        return partition;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public PartitionQosPolicy basicGetPartition() {
        return partition;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setPartition(PartitionQosPolicy newPartition) {
        PartitionQosPolicy oldPartition = partition;
        partition = newPartition;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER_SUBSCRIBER__PARTITION,
                    oldPartition, partition));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Transport getTransport() {
        if (transport != null && transport.eIsProxy()) {
            InternalEObject oldTransport = (InternalEObject) transport;
            transport = (Transport) eResolveProxy(oldTransport);
            if (transport != oldTransport) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT, oldTransport, transport));
                }
            }
        }
        return transport;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Transport basicGetTransport() {
        return transport;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setTransport(Transport newTransport) {
        Transport oldTransport = transport;
        transport = newTransport;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT,
                    oldTransport, transport));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY:
                if (resolve) {
                    return getEntity_factory();
                }
                return basicGetEntity_factory();
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION:
                if (resolve) {
                    return getPresentation();
                }
                return basicGetPresentation();
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA:
                if (resolve) {
                    return getGroup_data();
                }
                return basicGetGroup_data();
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PARTITION:
                if (resolve) {
                    return getPartition();
                }
                return basicGetPartition();
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT:
                if (resolve) {
                    return getTransport();
                }
                return basicGetTransport();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY:
                setEntity_factory((EntityFactoryQosPolicy) newValue);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION:
                setPresentation((PresentationQosPolicy) newValue);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA:
                setGroup_data((GroupDataQosPolicy) newValue);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PARTITION:
                setPartition((PartitionQosPolicy) newValue);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT:
                setTransport((Transport) newValue);
                return;
        }
        super.eSet(featureID, newValue);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void eUnset(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY:
                setEntity_factory((EntityFactoryQosPolicy) null);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION:
                setPresentation((PresentationQosPolicy) null);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA:
                setGroup_data((GroupDataQosPolicy) null);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PARTITION:
                setPartition((PartitionQosPolicy) null);
                return;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT:
                setTransport((Transport) null);
                return;
        }
        super.eUnset(featureID);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public boolean eIsSet(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY:
                return entity_factory != null;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION:
                return presentation != null;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA:
                return group_data != null;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__PARTITION:
                return partition != null;
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT:
                return transport != null;
        }
        return super.eIsSet(featureID);
    }

} // PublisherSubscriberImpl

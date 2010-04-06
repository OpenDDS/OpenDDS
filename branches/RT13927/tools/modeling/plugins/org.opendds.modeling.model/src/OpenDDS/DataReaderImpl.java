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
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Data Reader</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.DataReaderImpl#getTopic <em>Topic</em>}</li>
 *   <li>{@link OpenDDS.DataReaderImpl#getReader_data_lifecycle <em>Reader data lifecycle</em>}</li>
 *   <li>{@link OpenDDS.DataReaderImpl#getTransport_priority <em>Transport priority</em>}</li>
 *   <li>{@link OpenDDS.DataReaderImpl#getDurability_service <em>Durability service</em>}</li>
 *   <li>{@link OpenDDS.DataReaderImpl#getOwnership_strength <em>Ownership strength</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class DataReaderImpl extends DataReaderWriterImpl implements DataReader {
    /**
     * The cached value of the '{@link #getTopic() <em>Topic</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getTopic()
     * @generated
     * @ordered
     */
    protected TopicDescription topic;

    /**
     * The cached value of the '{@link #getReader_data_lifecycle() <em>Reader data lifecycle</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getReader_data_lifecycle()
     * @generated
     * @ordered
     */
    protected ReaderDataLifecycleQosPolicy reader_data_lifecycle;

    /**
     * The cached value of the '{@link #getTransport_priority() <em>Transport priority</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getTransport_priority()
     * @generated
     * @ordered
     */
    protected TransportPriorityQosPolicy transport_priority;

    /**
     * The cached value of the '{@link #getDurability_service() <em>Durability service</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getDurability_service()
     * @generated
     * @ordered
     */
    protected DurabilityServiceQosPolicy durability_service;

    /**
     * The cached value of the '{@link #getOwnership_strength() <em>Ownership strength</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getOwnership_strength()
     * @generated
     * @ordered
     */
    protected OwnershipStrengthQosPolicy ownership_strength;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected DataReaderImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.DATA_READER;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TopicDescription getTopic() {
        if (topic != null && topic.eIsProxy()) {
            InternalEObject oldTopic = (InternalEObject) topic;
            topic = (TopicDescription) eResolveProxy(oldTopic);
            if (topic != oldTopic) {
                if (eNotificationRequired())
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.DATA_READER__TOPIC,
                            oldTopic, topic));
            }
        }
        return topic;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TopicDescription basicGetTopic() {
        return topic;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setTopic(TopicDescription newTopic) {
        TopicDescription oldTopic = topic;
        topic = newTopic;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_READER__TOPIC, oldTopic, topic));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReaderDataLifecycleQosPolicy getReader_data_lifecycle() {
        if (reader_data_lifecycle != null && reader_data_lifecycle.eIsProxy()) {
            InternalEObject oldReader_data_lifecycle = (InternalEObject) reader_data_lifecycle;
            reader_data_lifecycle = (ReaderDataLifecycleQosPolicy) eResolveProxy(oldReader_data_lifecycle);
            if (reader_data_lifecycle != oldReader_data_lifecycle) {
                if (eNotificationRequired())
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DATA_READER__READER_DATA_LIFECYCLE, oldReader_data_lifecycle,
                            reader_data_lifecycle));
            }
        }
        return reader_data_lifecycle;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReaderDataLifecycleQosPolicy basicGetReader_data_lifecycle() {
        return reader_data_lifecycle;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setReader_data_lifecycle(ReaderDataLifecycleQosPolicy newReader_data_lifecycle) {
        ReaderDataLifecycleQosPolicy oldReader_data_lifecycle = reader_data_lifecycle;
        reader_data_lifecycle = newReader_data_lifecycle;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_READER__READER_DATA_LIFECYCLE,
                    oldReader_data_lifecycle, reader_data_lifecycle));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TransportPriorityQosPolicy getTransport_priority() {
        if (transport_priority != null && transport_priority.eIsProxy()) {
            InternalEObject oldTransport_priority = (InternalEObject) transport_priority;
            transport_priority = (TransportPriorityQosPolicy) eResolveProxy(oldTransport_priority);
            if (transport_priority != oldTransport_priority) {
                if (eNotificationRequired())
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DATA_READER__TRANSPORT_PRIORITY, oldTransport_priority, transport_priority));
            }
        }
        return transport_priority;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TransportPriorityQosPolicy basicGetTransport_priority() {
        return transport_priority;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setTransport_priority(TransportPriorityQosPolicy newTransport_priority) {
        TransportPriorityQosPolicy oldTransport_priority = transport_priority;
        transport_priority = newTransport_priority;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_READER__TRANSPORT_PRIORITY,
                    oldTransport_priority, transport_priority));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityServiceQosPolicy getDurability_service() {
        if (durability_service != null && durability_service.eIsProxy()) {
            InternalEObject oldDurability_service = (InternalEObject) durability_service;
            durability_service = (DurabilityServiceQosPolicy) eResolveProxy(oldDurability_service);
            if (durability_service != oldDurability_service) {
                if (eNotificationRequired())
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DATA_READER__DURABILITY_SERVICE, oldDurability_service, durability_service));
            }
        }
        return durability_service;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityServiceQosPolicy basicGetDurability_service() {
        return durability_service;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setDurability_service(DurabilityServiceQosPolicy newDurability_service) {
        DurabilityServiceQosPolicy oldDurability_service = durability_service;
        durability_service = newDurability_service;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_READER__DURABILITY_SERVICE,
                    oldDurability_service, durability_service));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipStrengthQosPolicy getOwnership_strength() {
        if (ownership_strength != null && ownership_strength.eIsProxy()) {
            InternalEObject oldOwnership_strength = (InternalEObject) ownership_strength;
            ownership_strength = (OwnershipStrengthQosPolicy) eResolveProxy(oldOwnership_strength);
            if (ownership_strength != oldOwnership_strength) {
                if (eNotificationRequired())
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DATA_READER__OWNERSHIP_STRENGTH, oldOwnership_strength, ownership_strength));
            }
        }
        return ownership_strength;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipStrengthQosPolicy basicGetOwnership_strength() {
        return ownership_strength;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setOwnership_strength(OwnershipStrengthQosPolicy newOwnership_strength) {
        OwnershipStrengthQosPolicy oldOwnership_strength = ownership_strength;
        ownership_strength = newOwnership_strength;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_READER__OWNERSHIP_STRENGTH,
                    oldOwnership_strength, ownership_strength));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.DATA_READER__TOPIC:
                if (resolve)
                    return getTopic();
                return basicGetTopic();
            case OpenDDSPackage.DATA_READER__READER_DATA_LIFECYCLE:
                if (resolve)
                    return getReader_data_lifecycle();
                return basicGetReader_data_lifecycle();
            case OpenDDSPackage.DATA_READER__TRANSPORT_PRIORITY:
                if (resolve)
                    return getTransport_priority();
                return basicGetTransport_priority();
            case OpenDDSPackage.DATA_READER__DURABILITY_SERVICE:
                if (resolve)
                    return getDurability_service();
                return basicGetDurability_service();
            case OpenDDSPackage.DATA_READER__OWNERSHIP_STRENGTH:
                if (resolve)
                    return getOwnership_strength();
                return basicGetOwnership_strength();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case OpenDDSPackage.DATA_READER__TOPIC:
                setTopic((TopicDescription) newValue);
                return;
            case OpenDDSPackage.DATA_READER__READER_DATA_LIFECYCLE:
                setReader_data_lifecycle((ReaderDataLifecycleQosPolicy) newValue);
                return;
            case OpenDDSPackage.DATA_READER__TRANSPORT_PRIORITY:
                setTransport_priority((TransportPriorityQosPolicy) newValue);
                return;
            case OpenDDSPackage.DATA_READER__DURABILITY_SERVICE:
                setDurability_service((DurabilityServiceQosPolicy) newValue);
                return;
            case OpenDDSPackage.DATA_READER__OWNERSHIP_STRENGTH:
                setOwnership_strength((OwnershipStrengthQosPolicy) newValue);
                return;
        }
        super.eSet(featureID, newValue);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public void eUnset(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.DATA_READER__TOPIC:
                setTopic((TopicDescription) null);
                return;
            case OpenDDSPackage.DATA_READER__READER_DATA_LIFECYCLE:
                setReader_data_lifecycle((ReaderDataLifecycleQosPolicy) null);
                return;
            case OpenDDSPackage.DATA_READER__TRANSPORT_PRIORITY:
                setTransport_priority((TransportPriorityQosPolicy) null);
                return;
            case OpenDDSPackage.DATA_READER__DURABILITY_SERVICE:
                setDurability_service((DurabilityServiceQosPolicy) null);
                return;
            case OpenDDSPackage.DATA_READER__OWNERSHIP_STRENGTH:
                setOwnership_strength((OwnershipStrengthQosPolicy) null);
                return;
        }
        super.eUnset(featureID);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public boolean eIsSet(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.DATA_READER__TOPIC:
                return topic != null;
            case OpenDDSPackage.DATA_READER__READER_DATA_LIFECYCLE:
                return reader_data_lifecycle != null;
            case OpenDDSPackage.DATA_READER__TRANSPORT_PRIORITY:
                return transport_priority != null;
            case OpenDDSPackage.DATA_READER__DURABILITY_SERVICE:
                return durability_service != null;
            case OpenDDSPackage.DATA_READER__OWNERSHIP_STRENGTH:
                return ownership_strength != null;
        }
        return super.eIsSet(featureID);
    }

} //DataReaderImpl

/**
 * <copyright>
 * </copyright>
 *
 * $Id$
 */
package org.opendds.modeling.model.opendds.impl;

import java.util.Collection;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotificationChain;

import org.eclipse.emf.common.util.EList;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;

import org.eclipse.emf.ecore.impl.ENotificationImpl;

import org.eclipse.emf.ecore.util.EObjectContainmentEList;
import org.eclipse.emf.ecore.util.InternalEList;

import org.opendds.modeling.model.dcps.DCPSPackage;
import org.opendds.modeling.model.dcps.DataWriter;
import org.opendds.modeling.model.dcps.Publisher;
import org.opendds.modeling.model.dcps.PublisherSubscriber;

import org.opendds.modeling.model.domain.DomainEntity;
import org.opendds.modeling.model.domain.DomainPackage;
import org.opendds.modeling.model.domain.QosProperty;

import org.opendds.modeling.model.opendds.OpenDDSPackage;
import org.opendds.modeling.model.opendds.publisher;

import org.opendds.modeling.model.qos.EntityFactoryQosPolicy;
import org.opendds.modeling.model.qos.GroupDataQosPolicy;
import org.opendds.modeling.model.qos.PartitionQosPolicy;
import org.opendds.modeling.model.qos.PresentationQosPolicy;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>publisher</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getQosPolicy <em>Qos Policy</em>}</li>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getEntity_factory <em>Entity factory</em>}</li>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getGroup_data <em>Group data</em>}</li>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getPresentation <em>Presentation</em>}</li>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getPartition <em>Partition</em>}</li>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getTransportId <em>Transport Id</em>}</li>
 *   <li>{@link org.opendds.modeling.model.opendds.impl.publisherImpl#getWriters <em>Writers</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class publisherImpl extends ddsPropertyImpl implements publisher {
	/**
	 * The cached value of the '{@link #getQosPolicy() <em>Qos Policy</em>}' containment reference list.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getQosPolicy()
	 * @generated
	 * @ordered
	 */
	protected EList<QosProperty> qosPolicy;

	/**
	 * The cached value of the '{@link #getEntity_factory() <em>Entity factory</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getEntity_factory()
	 * @generated
	 * @ordered
	 */
	protected EntityFactoryQosPolicy entity_factory;

	/**
	 * The cached value of the '{@link #getGroup_data() <em>Group data</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getGroup_data()
	 * @generated
	 * @ordered
	 */
	protected GroupDataQosPolicy group_data;

	/**
	 * The cached value of the '{@link #getPresentation() <em>Presentation</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getPresentation()
	 * @generated
	 * @ordered
	 */
	protected PresentationQosPolicy presentation;

	/**
	 * The cached value of the '{@link #getPartition() <em>Partition</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getPartition()
	 * @generated
	 * @ordered
	 */
	protected PartitionQosPolicy partition;

	/**
	 * The default value of the '{@link #getTransportId() <em>Transport Id</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getTransportId()
	 * @generated
	 * @ordered
	 */
	protected static final int TRANSPORT_ID_EDEFAULT = -1;

	/**
	 * The cached value of the '{@link #getTransportId() <em>Transport Id</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getTransportId()
	 * @generated
	 * @ordered
	 */
	protected int transportId = TRANSPORT_ID_EDEFAULT;

	/**
	 * The cached value of the '{@link #getWriters() <em>Writers</em>}' containment reference list.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getWriters()
	 * @generated
	 * @ordered
	 */
	protected EList<DataWriter> writers;

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected publisherImpl() {
		super();
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected EClass eStaticClass() {
		return OpenDDSPackage.Literals.PUBLISHER;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public EList<QosProperty> getQosPolicy() {
		if (qosPolicy == null) {
			qosPolicy = new EObjectContainmentEList<QosProperty>(QosProperty.class, this, OpenDDSPackage.PUBLISHER__QOS_POLICY);
		}
		return qosPolicy;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public EntityFactoryQosPolicy getEntity_factory() {
		if (entity_factory != null && entity_factory.eIsProxy()) {
			InternalEObject oldEntity_factory = (InternalEObject)entity_factory;
			entity_factory = (EntityFactoryQosPolicy)eResolveProxy(oldEntity_factory);
			if (entity_factory != oldEntity_factory) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.PUBLISHER__ENTITY_FACTORY, oldEntity_factory, entity_factory));
			}
		}
		return entity_factory;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public EntityFactoryQosPolicy basicGetEntity_factory() {
		return entity_factory;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setEntity_factory(EntityFactoryQosPolicy newEntity_factory) {
		EntityFactoryQosPolicy oldEntity_factory = entity_factory;
		entity_factory = newEntity_factory;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER__ENTITY_FACTORY, oldEntity_factory, entity_factory));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public GroupDataQosPolicy getGroup_data() {
		if (group_data != null && group_data.eIsProxy()) {
			InternalEObject oldGroup_data = (InternalEObject)group_data;
			group_data = (GroupDataQosPolicy)eResolveProxy(oldGroup_data);
			if (group_data != oldGroup_data) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.PUBLISHER__GROUP_DATA, oldGroup_data, group_data));
			}
		}
		return group_data;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public GroupDataQosPolicy basicGetGroup_data() {
		return group_data;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setGroup_data(GroupDataQosPolicy newGroup_data) {
		GroupDataQosPolicy oldGroup_data = group_data;
		group_data = newGroup_data;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER__GROUP_DATA, oldGroup_data, group_data));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public PresentationQosPolicy getPresentation() {
		if (presentation != null && presentation.eIsProxy()) {
			InternalEObject oldPresentation = (InternalEObject)presentation;
			presentation = (PresentationQosPolicy)eResolveProxy(oldPresentation);
			if (presentation != oldPresentation) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.PUBLISHER__PRESENTATION, oldPresentation, presentation));
			}
		}
		return presentation;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public PresentationQosPolicy basicGetPresentation() {
		return presentation;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setPresentation(PresentationQosPolicy newPresentation) {
		PresentationQosPolicy oldPresentation = presentation;
		presentation = newPresentation;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER__PRESENTATION, oldPresentation, presentation));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public PartitionQosPolicy getPartition() {
		if (partition != null && partition.eIsProxy()) {
			InternalEObject oldPartition = (InternalEObject)partition;
			partition = (PartitionQosPolicy)eResolveProxy(oldPartition);
			if (partition != oldPartition) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.PUBLISHER__PARTITION, oldPartition, partition));
			}
		}
		return partition;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public PartitionQosPolicy basicGetPartition() {
		return partition;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setPartition(PartitionQosPolicy newPartition) {
		PartitionQosPolicy oldPartition = partition;
		partition = newPartition;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER__PARTITION, oldPartition, partition));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public int getTransportId() {
		return transportId;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setTransportId(int newTransportId) {
		int oldTransportId = transportId;
		transportId = newTransportId;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.PUBLISHER__TRANSPORT_ID, oldTransportId, transportId));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public EList<DataWriter> getWriters() {
		if (writers == null) {
			writers = new EObjectContainmentEList<DataWriter>(DataWriter.class, this, OpenDDSPackage.PUBLISHER__WRITERS);
		}
		return writers;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
		switch (featureID) {
			case OpenDDSPackage.PUBLISHER__QOS_POLICY:
				return ((InternalEList<?>)getQosPolicy()).basicRemove(otherEnd, msgs);
			case OpenDDSPackage.PUBLISHER__WRITERS:
				return ((InternalEList<?>)getWriters()).basicRemove(otherEnd, msgs);
		}
		return super.eInverseRemove(otherEnd, featureID, msgs);
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Object eGet(int featureID, boolean resolve, boolean coreType) {
		switch (featureID) {
			case OpenDDSPackage.PUBLISHER__QOS_POLICY:
				return getQosPolicy();
			case OpenDDSPackage.PUBLISHER__ENTITY_FACTORY:
				if (resolve) return getEntity_factory();
				return basicGetEntity_factory();
			case OpenDDSPackage.PUBLISHER__GROUP_DATA:
				if (resolve) return getGroup_data();
				return basicGetGroup_data();
			case OpenDDSPackage.PUBLISHER__PRESENTATION:
				if (resolve) return getPresentation();
				return basicGetPresentation();
			case OpenDDSPackage.PUBLISHER__PARTITION:
				if (resolve) return getPartition();
				return basicGetPartition();
			case OpenDDSPackage.PUBLISHER__TRANSPORT_ID:
				return getTransportId();
			case OpenDDSPackage.PUBLISHER__WRITERS:
				return getWriters();
		}
		return super.eGet(featureID, resolve, coreType);
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@SuppressWarnings("unchecked")
	@Override
	public void eSet(int featureID, Object newValue) {
		switch (featureID) {
			case OpenDDSPackage.PUBLISHER__QOS_POLICY:
				getQosPolicy().clear();
				getQosPolicy().addAll((Collection<? extends QosProperty>)newValue);
				return;
			case OpenDDSPackage.PUBLISHER__ENTITY_FACTORY:
				setEntity_factory((EntityFactoryQosPolicy)newValue);
				return;
			case OpenDDSPackage.PUBLISHER__GROUP_DATA:
				setGroup_data((GroupDataQosPolicy)newValue);
				return;
			case OpenDDSPackage.PUBLISHER__PRESENTATION:
				setPresentation((PresentationQosPolicy)newValue);
				return;
			case OpenDDSPackage.PUBLISHER__PARTITION:
				setPartition((PartitionQosPolicy)newValue);
				return;
			case OpenDDSPackage.PUBLISHER__TRANSPORT_ID:
				setTransportId((Integer)newValue);
				return;
			case OpenDDSPackage.PUBLISHER__WRITERS:
				getWriters().clear();
				getWriters().addAll((Collection<? extends DataWriter>)newValue);
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
			case OpenDDSPackage.PUBLISHER__QOS_POLICY:
				getQosPolicy().clear();
				return;
			case OpenDDSPackage.PUBLISHER__ENTITY_FACTORY:
				setEntity_factory((EntityFactoryQosPolicy)null);
				return;
			case OpenDDSPackage.PUBLISHER__GROUP_DATA:
				setGroup_data((GroupDataQosPolicy)null);
				return;
			case OpenDDSPackage.PUBLISHER__PRESENTATION:
				setPresentation((PresentationQosPolicy)null);
				return;
			case OpenDDSPackage.PUBLISHER__PARTITION:
				setPartition((PartitionQosPolicy)null);
				return;
			case OpenDDSPackage.PUBLISHER__TRANSPORT_ID:
				setTransportId(TRANSPORT_ID_EDEFAULT);
				return;
			case OpenDDSPackage.PUBLISHER__WRITERS:
				getWriters().clear();
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
			case OpenDDSPackage.PUBLISHER__QOS_POLICY:
				return qosPolicy != null && !qosPolicy.isEmpty();
			case OpenDDSPackage.PUBLISHER__ENTITY_FACTORY:
				return entity_factory != null;
			case OpenDDSPackage.PUBLISHER__GROUP_DATA:
				return group_data != null;
			case OpenDDSPackage.PUBLISHER__PRESENTATION:
				return presentation != null;
			case OpenDDSPackage.PUBLISHER__PARTITION:
				return partition != null;
			case OpenDDSPackage.PUBLISHER__TRANSPORT_ID:
				return transportId != TRANSPORT_ID_EDEFAULT;
			case OpenDDSPackage.PUBLISHER__WRITERS:
				return writers != null && !writers.isEmpty();
		}
		return super.eIsSet(featureID);
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public int eBaseStructuralFeatureID(int derivedFeatureID, Class<?> baseClass) {
		if (baseClass == DomainEntity.class) {
			switch (derivedFeatureID) {
				case OpenDDSPackage.PUBLISHER__QOS_POLICY: return DomainPackage.DOMAIN_ENTITY__QOS_POLICY;
				default: return -1;
			}
		}
		if (baseClass == PublisherSubscriber.class) {
			switch (derivedFeatureID) {
				case OpenDDSPackage.PUBLISHER__ENTITY_FACTORY: return DCPSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY;
				case OpenDDSPackage.PUBLISHER__GROUP_DATA: return DCPSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA;
				case OpenDDSPackage.PUBLISHER__PRESENTATION: return DCPSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION;
				case OpenDDSPackage.PUBLISHER__PARTITION: return DCPSPackage.PUBLISHER_SUBSCRIBER__PARTITION;
				case OpenDDSPackage.PUBLISHER__TRANSPORT_ID: return DCPSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT_ID;
				default: return -1;
			}
		}
		if (baseClass == Publisher.class) {
			switch (derivedFeatureID) {
				case OpenDDSPackage.PUBLISHER__WRITERS: return DCPSPackage.PUBLISHER__WRITERS;
				default: return -1;
			}
		}
		return super.eBaseStructuralFeatureID(derivedFeatureID, baseClass);
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public int eDerivedStructuralFeatureID(int baseFeatureID, Class<?> baseClass) {
		if (baseClass == DomainEntity.class) {
			switch (baseFeatureID) {
				case DomainPackage.DOMAIN_ENTITY__QOS_POLICY: return OpenDDSPackage.PUBLISHER__QOS_POLICY;
				default: return -1;
			}
		}
		if (baseClass == PublisherSubscriber.class) {
			switch (baseFeatureID) {
				case DCPSPackage.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY: return OpenDDSPackage.PUBLISHER__ENTITY_FACTORY;
				case DCPSPackage.PUBLISHER_SUBSCRIBER__GROUP_DATA: return OpenDDSPackage.PUBLISHER__GROUP_DATA;
				case DCPSPackage.PUBLISHER_SUBSCRIBER__PRESENTATION: return OpenDDSPackage.PUBLISHER__PRESENTATION;
				case DCPSPackage.PUBLISHER_SUBSCRIBER__PARTITION: return OpenDDSPackage.PUBLISHER__PARTITION;
				case DCPSPackage.PUBLISHER_SUBSCRIBER__TRANSPORT_ID: return OpenDDSPackage.PUBLISHER__TRANSPORT_ID;
				default: return -1;
			}
		}
		if (baseClass == Publisher.class) {
			switch (baseFeatureID) {
				case DCPSPackage.PUBLISHER__WRITERS: return OpenDDSPackage.PUBLISHER__WRITERS;
				default: return -1;
			}
		}
		return super.eDerivedStructuralFeatureID(baseFeatureID, baseClass);
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public String toString() {
		if (eIsProxy()) return super.toString();

		StringBuffer result = new StringBuffer(super.toString());
		result.append(" (transportId: ");
		result.append(transportId);
		result.append(')');
		return result.toString();
	}

	/**
	 * @generated NOT
	 */
	@Override
	public EList<org.opendds.modeling.model.qos.QosPolicy> getPolicies() {
		return com.ociweb.emf.util.ReferencesFinder.findInstancesOf(org.opendds.modeling.model.qos.QosPolicy.class, this);
	}

} //publisherImpl

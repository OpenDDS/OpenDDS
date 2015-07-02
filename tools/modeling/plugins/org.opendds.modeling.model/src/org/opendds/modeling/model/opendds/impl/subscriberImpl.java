/**
 * <copyright>
 * </copyright>
 *
 */
package org.opendds.modeling.model.opendds.impl;

import java.util.Collection;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotificationChain;

import org.eclipse.emf.common.util.EList;

import org.eclipse.emf.ecore.EClass;
import org.opendds.modeling.model.dcps.impl.SubscriberImpl;
import org.eclipse.emf.ecore.InternalEObject;

import org.eclipse.emf.ecore.impl.ENotificationImpl;

import org.eclipse.emf.ecore.util.EObjectContainmentEList;
import org.eclipse.emf.ecore.util.InternalEList;

import org.opendds.modeling.model.dcps.DCPSPackage;
import org.opendds.modeling.model.dcps.DataReader;
import org.opendds.modeling.model.dcps.PublisherSubscriber;
import org.opendds.modeling.model.dcps.Subscriber;

import org.opendds.modeling.model.domain.DomainEntity;
import org.opendds.modeling.model.domain.DomainPackage;
import org.opendds.modeling.model.domain.QosProperty;

import org.opendds.modeling.model.opendds.OpenDDSPackage;
import org.opendds.modeling.model.opendds.subscriber;

import org.opendds.modeling.model.qos.EntityFactoryQosPolicy;
import org.opendds.modeling.model.qos.GroupDataQosPolicy;
import org.opendds.modeling.model.qos.PartitionQosPolicy;
import org.opendds.modeling.model.qos.PresentationQosPolicy;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>subscriber</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * </p>
 *
 * @generated
 */
public class subscriberImpl extends SubscriberImpl implements subscriber {
	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected subscriberImpl() {
		super();
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected EClass eStaticClass() {
		return OpenDDSPackage.Literals.SUBSCRIBER;
	}

	/**
	 * @generated NOT
	 */
	@Override
	public EList<org.opendds.modeling.model.qos.QosPolicy> getPolicies() {
		return com.ociweb.emf.util.ReferencesFinder.findInstancesOf(
				org.opendds.modeling.model.qos.QosPolicy.class, this);
	}

} //subscriberImpl

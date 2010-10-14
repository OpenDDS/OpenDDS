/**
 * <copyright>
 * </copyright>
 *
 * $Id$
 */
package org.opendds.modeling.model.domain.impl;

import java.util.Collection;

import org.eclipse.emf.common.notify.NotificationChain;

import org.eclipse.emf.common.util.EList;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;

import org.eclipse.emf.ecore.util.EObjectContainmentEList;
import org.eclipse.emf.ecore.util.InternalEList;

import org.opendds.modeling.model.core.impl.EntityImpl;

import org.opendds.modeling.model.domain.DomainEntity;
import org.opendds.modeling.model.domain.DomainPackage;
import org.opendds.modeling.model.domain.QosProperty;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Entity</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.domain.impl.DomainEntityImpl#getQosPolicy <em>Qos Policy</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class DomainEntityImpl extends EntityImpl implements DomainEntity {
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
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected DomainEntityImpl() {
		super();
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected EClass eStaticClass() {
		return DomainPackage.Literals.DOMAIN_ENTITY;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public EList<QosProperty> getQosPolicy() {
		if (qosPolicy == null) {
			qosPolicy = new EObjectContainmentEList<QosProperty>(QosProperty.class, this, DomainPackage.DOMAIN_ENTITY__QOS_POLICY);
		}
		return qosPolicy;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
		switch (featureID) {
			case DomainPackage.DOMAIN_ENTITY__QOS_POLICY:
				return ((InternalEList<?>)getQosPolicy()).basicRemove(otherEnd, msgs);
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
			case DomainPackage.DOMAIN_ENTITY__QOS_POLICY:
				return getQosPolicy();
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
			case DomainPackage.DOMAIN_ENTITY__QOS_POLICY:
				getQosPolicy().clear();
				getQosPolicy().addAll((Collection<? extends QosProperty>)newValue);
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
			case DomainPackage.DOMAIN_ENTITY__QOS_POLICY:
				getQosPolicy().clear();
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
			case DomainPackage.DOMAIN_ENTITY__QOS_POLICY:
				return qosPolicy != null && !qosPolicy.isEmpty();
		}
		return super.eIsSet(featureID);
	}

	/**
	 * @generated NOT
	 */
	@Override
	public EList<org.opendds.modeling.model.qos.QosPolicy> getPolicies() {
		return com.ociweb.emf.util.ReferencesFinder.findInstancesOf(org.opendds.modeling.model.qos.QosPolicy.class, this);
	}

} //DomainEntityImpl

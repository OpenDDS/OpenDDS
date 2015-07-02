/**
 * <copyright>
 * </copyright>
 *
 */
package org.opendds.modeling.model.domain.impl;

import org.eclipse.emf.common.notify.Notification;
import java.util.Collection;

import org.eclipse.emf.common.notify.NotificationChain;

import org.eclipse.emf.common.util.EList;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.impl.ENotificationImpl;
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
 *   <li>{@link org.opendds.modeling.model.domain.impl.DomainEntityImpl#getTransportConfig <em>Transport Config</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class DomainEntityImpl extends EntityImpl implements DomainEntity {
	/**
	 * The default value of the '{@link #getTransportConfig() <em>Transport Config</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getTransportConfig()
	 * @generated
	 * @ordered
	 */
	protected static final String TRANSPORT_CONFIG_EDEFAULT = null;
	/**
	 * The cached value of the '{@link #getTransportConfig() <em>Transport Config</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getTransportConfig()
	 * @generated
	 * @ordered
	 */
	protected String transportConfig = TRANSPORT_CONFIG_EDEFAULT;

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
	public String getTransportConfig() {
		return transportConfig;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setTransportConfig(String newTransportConfig) {
		String oldTransportConfig = transportConfig;
		transportConfig = newTransportConfig;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					DomainPackage.DOMAIN_ENTITY__TRANSPORT_CONFIG,
					oldTransportConfig, transportConfig));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Object eGet(int featureID, boolean resolve, boolean coreType) {
		switch (featureID) {
		case DomainPackage.DOMAIN_ENTITY__TRANSPORT_CONFIG:
			return getTransportConfig();
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
		case DomainPackage.DOMAIN_ENTITY__TRANSPORT_CONFIG:
			setTransportConfig((String) newValue);
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
		case DomainPackage.DOMAIN_ENTITY__TRANSPORT_CONFIG:
			setTransportConfig(TRANSPORT_CONFIG_EDEFAULT);
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
		case DomainPackage.DOMAIN_ENTITY__TRANSPORT_CONFIG:
			return TRANSPORT_CONFIG_EDEFAULT == null ? transportConfig != null
					: !TRANSPORT_CONFIG_EDEFAULT.equals(transportConfig);
		}
		return super.eIsSet(featureID);
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public String toString() {
		if (eIsProxy())
			return super.toString();

		StringBuffer result = new StringBuffer(super.toString());
		result.append(" (transportConfig: ");
		result.append(transportConfig);
		result.append(')');
		return result.toString();
	}

	/**
	 * @generated NOT
	 */
	@Override
	public EList<org.opendds.modeling.model.qos.QosPolicy> getPolicies() {
		return com.ociweb.emf.util.ReferencesFinder.findInstancesOf(
				org.opendds.modeling.model.qos.QosPolicy.class, this);
	}

} //DomainEntityImpl

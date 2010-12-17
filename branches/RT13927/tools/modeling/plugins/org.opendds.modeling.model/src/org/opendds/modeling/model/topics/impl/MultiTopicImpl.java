/**
 * <copyright>
 * </copyright>
 *
 * $Id$
 */
package org.opendds.modeling.model.topics.impl;

import org.eclipse.emf.common.notify.Notification;

import org.eclipse.emf.ecore.EClass;

import org.eclipse.emf.ecore.impl.ENotificationImpl;

import org.opendds.modeling.model.topics.MultiTopic;
import org.opendds.modeling.model.topics.TopicsPackage;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Multi Topic</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.topics.impl.MultiTopicImpl#getSubscription_expression <em>Subscription expression</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class MultiTopicImpl extends TopicDescriptionImpl implements MultiTopic {
	/**
	 * The default value of the '{@link #getSubscription_expression() <em>Subscription expression</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getSubscription_expression()
	 * @generated
	 * @ordered
	 */
	protected static final String SUBSCRIPTION_EXPRESSION_EDEFAULT = null;

	/**
	 * The cached value of the '{@link #getSubscription_expression() <em>Subscription expression</em>}' attribute.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getSubscription_expression()
	 * @generated
	 * @ordered
	 */
	protected String subscription_expression = SUBSCRIPTION_EXPRESSION_EDEFAULT;

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected MultiTopicImpl() {
		super();
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected EClass eStaticClass() {
		return TopicsPackage.Literals.MULTI_TOPIC;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public String getSubscription_expression() {
		return subscription_expression;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setSubscription_expression(String newSubscription_expression) {
		String oldSubscription_expression = subscription_expression;
		subscription_expression = newSubscription_expression;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET, TopicsPackage.MULTI_TOPIC__SUBSCRIPTION_EXPRESSION, oldSubscription_expression, subscription_expression));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Object eGet(int featureID, boolean resolve, boolean coreType) {
		switch (featureID) {
			case TopicsPackage.MULTI_TOPIC__SUBSCRIPTION_EXPRESSION:
				return getSubscription_expression();
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
			case TopicsPackage.MULTI_TOPIC__SUBSCRIPTION_EXPRESSION:
				setSubscription_expression((String)newValue);
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
			case TopicsPackage.MULTI_TOPIC__SUBSCRIPTION_EXPRESSION:
				setSubscription_expression(SUBSCRIPTION_EXPRESSION_EDEFAULT);
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
			case TopicsPackage.MULTI_TOPIC__SUBSCRIPTION_EXPRESSION:
				return SUBSCRIPTION_EXPRESSION_EDEFAULT == null ? subscription_expression != null : !SUBSCRIPTION_EXPRESSION_EDEFAULT.equals(subscription_expression);
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
		if (eIsProxy()) return super.toString();

		StringBuffer result = new StringBuffer(super.toString());
		result.append(" (subscription_expression: ");
		result.append(subscription_expression);
		result.append(')');
		return result.toString();
	}

	/**
	 * @generated NOT
	 */
	@Override
	public org.eclipse.emf.common.util.EList<org.opendds.modeling.model.types.Type> getTypes() {
		return com.ociweb.emf.util.ReferencesFinder.findInstancesOf(org.opendds.modeling.model.types.Type.class, this);		
	}

} //MultiTopicImpl

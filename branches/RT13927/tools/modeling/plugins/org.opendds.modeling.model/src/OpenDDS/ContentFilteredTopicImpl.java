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

import org.eclipse.emf.ecore.impl.ENotificationImpl;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Content Filtered Topic</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.ContentFilteredTopicImpl#getFilter_expression <em>Filter expression</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class ContentFilteredTopicImpl extends TopicDescriptionImpl implements ContentFilteredTopic {
    /**
     * The default value of the '{@link #getFilter_expression() <em>Filter expression</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getFilter_expression()
     * @generated
     * @ordered
     */
    protected static final String FILTER_EXPRESSION_EDEFAULT = null;

    /**
     * The cached value of the '{@link #getFilter_expression() <em>Filter expression</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getFilter_expression()
     * @generated
     * @ordered
     */
    protected String filter_expression = FILTER_EXPRESSION_EDEFAULT;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected ContentFilteredTopicImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return ModelPackage.Literals.CONTENT_FILTERED_TOPIC;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String getFilter_expression() {
        return filter_expression;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setFilter_expression(String newFilter_expression) {
        String oldFilter_expression = filter_expression;
        filter_expression = newFilter_expression;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    ModelPackage.CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION, oldFilter_expression, filter_expression));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case ModelPackage.CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION:
                return getFilter_expression();
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
            case ModelPackage.CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION:
                setFilter_expression((String) newValue);
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
            case ModelPackage.CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION:
                setFilter_expression(FILTER_EXPRESSION_EDEFAULT);
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
            case ModelPackage.CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION:
                return FILTER_EXPRESSION_EDEFAULT == null ? filter_expression != null : !FILTER_EXPRESSION_EDEFAULT
                        .equals(filter_expression);
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
        result.append(" (filter_expression: ");
        result.append(filter_expression);
        result.append(')');
        return result.toString();
    }

} //ContentFilteredTopicImpl

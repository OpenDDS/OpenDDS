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
 * <!-- begin-user-doc --> An implementation of the model object '
 * <em><b>Destination Order Qos Policy</b></em>'. <!-- end-user-doc
 * -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.DestinationOrderQosPolicyImpl#getKind <em>Kind
 * </em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class DestinationOrderQosPolicyImpl extends QosPolicyImpl implements DestinationOrderQosPolicy {
    /**
     * The default value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected static final DestinationOrderQosPolicyKind KIND_EDEFAULT = DestinationOrderQosPolicyKind.BY_RECEPTION_TIMESTAMP;

    /**
     * The cached value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected DestinationOrderQosPolicyKind kind = KIND_EDEFAULT;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    protected DestinationOrderQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.DESTINATION_ORDER_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public DestinationOrderQosPolicyKind getKind() {
        return kind;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public void setKind(DestinationOrderQosPolicyKind newKind) {
        DestinationOrderQosPolicyKind oldKind = kind;
        kind = newKind == null ? KIND_EDEFAULT : newKind;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DESTINATION_ORDER_QOS_POLICY__KIND,
                    oldKind, kind));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.DESTINATION_ORDER_QOS_POLICY__KIND:
                return getKind();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case OpenDDSPackage.DESTINATION_ORDER_QOS_POLICY__KIND:
                setKind((DestinationOrderQosPolicyKind) newValue);
                return;
        }
        super.eSet(featureID, newValue);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    public void eUnset(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.DESTINATION_ORDER_QOS_POLICY__KIND:
                setKind(KIND_EDEFAULT);
                return;
        }
        super.eUnset(featureID);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    public boolean eIsSet(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.DESTINATION_ORDER_QOS_POLICY__KIND:
                return kind != KIND_EDEFAULT;
        }
        return super.eIsSet(featureID);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    public String toString() {
        if (eIsProxy()) {
            return super.toString();
        }

        StringBuffer result = new StringBuffer(super.toString());
        result.append(" (kind: ");
        result.append(kind);
        result.append(')');
        return result.toString();
    }

} // DestinationOrderQosPolicyImpl

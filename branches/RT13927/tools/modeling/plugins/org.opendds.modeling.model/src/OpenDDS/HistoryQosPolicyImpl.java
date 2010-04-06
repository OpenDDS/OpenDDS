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
 * An implementation of the model object '<em><b>History Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.HistoryQosPolicyImpl#getDepth <em>Depth</em>}</li>
 *   <li>{@link OpenDDS.HistoryQosPolicyImpl#getKind <em>Kind</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class HistoryQosPolicyImpl extends QosPolicyImpl implements HistoryQosPolicy {
    /**
     * The default value of the '{@link #getDepth() <em>Depth</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getDepth()
     * @generated
     * @ordered
     */
    protected static final long DEPTH_EDEFAULT = 0L;

    /**
     * The cached value of the '{@link #getDepth() <em>Depth</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getDepth()
     * @generated
     * @ordered
     */
    protected long depth = DEPTH_EDEFAULT;

    /**
     * The default value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected static final HistoryQosPolicyKind KIND_EDEFAULT = HistoryQosPolicyKind.KEEP_LAST;

    /**
     * The cached value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected HistoryQosPolicyKind kind = KIND_EDEFAULT;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected HistoryQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.HISTORY_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public long getDepth() {
        return depth;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setDepth(long newDepth) {
        long oldDepth = depth;
        depth = newDepth;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.HISTORY_QOS_POLICY__DEPTH, oldDepth,
                    depth));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public HistoryQosPolicyKind getKind() {
        return kind;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setKind(HistoryQosPolicyKind newKind) {
        HistoryQosPolicyKind oldKind = kind;
        kind = newKind == null ? KIND_EDEFAULT : newKind;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.HISTORY_QOS_POLICY__KIND, oldKind,
                    kind));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.HISTORY_QOS_POLICY__DEPTH:
                return getDepth();
            case OpenDDSPackage.HISTORY_QOS_POLICY__KIND:
                return getKind();
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
            case OpenDDSPackage.HISTORY_QOS_POLICY__DEPTH:
                setDepth((Long) newValue);
                return;
            case OpenDDSPackage.HISTORY_QOS_POLICY__KIND:
                setKind((HistoryQosPolicyKind) newValue);
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
            case OpenDDSPackage.HISTORY_QOS_POLICY__DEPTH:
                setDepth(DEPTH_EDEFAULT);
                return;
            case OpenDDSPackage.HISTORY_QOS_POLICY__KIND:
                setKind(KIND_EDEFAULT);
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
            case OpenDDSPackage.HISTORY_QOS_POLICY__DEPTH:
                return depth != DEPTH_EDEFAULT;
            case OpenDDSPackage.HISTORY_QOS_POLICY__KIND:
                return kind != KIND_EDEFAULT;
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
        result.append(" (depth: ");
        result.append(depth);
        result.append(", kind: ");
        result.append(kind);
        result.append(')');
        return result.toString();
    }

} //HistoryQosPolicyImpl

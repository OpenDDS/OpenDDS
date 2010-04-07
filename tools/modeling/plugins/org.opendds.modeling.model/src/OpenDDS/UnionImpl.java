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
 * <em><b>Union</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.UnionImpl#getSwitch <em>Switch</em>}</li>
 *   <li>{@link OpenDDS.UnionImpl#getCases <em>Cases</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class UnionImpl extends ConstructedTopicTypeImpl implements Union {
    /**
     * The cached value of the '{@link #getSwitch() <em>Switch</em>}' reference.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @see #getSwitch()
     * @generated
     * @ordered
     */
    protected TopicField switch_;

    /**
     * The cached value of the '{@link #getCases() <em>Cases</em>}' reference.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @see #getCases()
     * @generated
     * @ordered
     */
    protected Case cases;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    protected UnionImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.UNION;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public TopicField getSwitch() {
        if (switch_ != null && switch_.eIsProxy()) {
            InternalEObject oldSwitch = (InternalEObject) switch_;
            switch_ = (TopicField) eResolveProxy(oldSwitch);
            if (switch_ != oldSwitch) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.UNION__SWITCH, oldSwitch,
                            switch_));
                }
            }
        }
        return switch_;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public TopicField basicGetSwitch() {
        return switch_;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public void setSwitch(TopicField newSwitch) {
        TopicField oldSwitch = switch_;
        switch_ = newSwitch;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.UNION__SWITCH, oldSwitch, switch_));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public Case getCases() {
        if (cases != null && cases.eIsProxy()) {
            InternalEObject oldCases = (InternalEObject) cases;
            cases = (Case) eResolveProxy(oldCases);
            if (cases != oldCases) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.UNION__CASES, oldCases,
                            cases));
                }
            }
        }
        return cases;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public Case basicGetCases() {
        return cases;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    public void setCases(Case newCases) {
        Case oldCases = cases;
        cases = newCases;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.UNION__CASES, oldCases, cases));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.UNION__SWITCH:
                if (resolve) {
                    return getSwitch();
                }
                return basicGetSwitch();
            case OpenDDSPackage.UNION__CASES:
                if (resolve) {
                    return getCases();
                }
                return basicGetCases();
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
            case OpenDDSPackage.UNION__SWITCH:
                setSwitch((TopicField) newValue);
                return;
            case OpenDDSPackage.UNION__CASES:
                setCases((Case) newValue);
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
            case OpenDDSPackage.UNION__SWITCH:
                setSwitch((TopicField) null);
                return;
            case OpenDDSPackage.UNION__CASES:
                setCases((Case) null);
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
            case OpenDDSPackage.UNION__SWITCH:
                return switch_ != null;
            case OpenDDSPackage.UNION__CASES:
                return cases != null;
        }
        return super.eIsSet(featureID);
    }

} // UnionImpl

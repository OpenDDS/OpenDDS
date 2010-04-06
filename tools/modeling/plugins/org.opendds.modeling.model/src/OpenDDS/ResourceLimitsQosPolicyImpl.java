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
 * An implementation of the model object '<em><b>Resource Limits Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.ResourceLimitsQosPolicyImpl#getMax_instances <em>Max instances</em>}</li>
 *   <li>{@link OpenDDS.ResourceLimitsQosPolicyImpl#getMax_samples <em>Max samples</em>}</li>
 *   <li>{@link OpenDDS.ResourceLimitsQosPolicyImpl#getMax_samples_per_instance <em>Max samples per instance</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class ResourceLimitsQosPolicyImpl extends QosPolicyImpl implements ResourceLimitsQosPolicy {
    /**
     * The default value of the '{@link #getMax_instances() <em>Max instances</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_instances()
     * @generated
     * @ordered
     */
    protected static final long MAX_INSTANCES_EDEFAULT = 0L;

    /**
     * The cached value of the '{@link #getMax_instances() <em>Max instances</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_instances()
     * @generated
     * @ordered
     */
    protected long max_instances = MAX_INSTANCES_EDEFAULT;

    /**
     * The default value of the '{@link #getMax_samples() <em>Max samples</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_samples()
     * @generated
     * @ordered
     */
    protected static final long MAX_SAMPLES_EDEFAULT = 0L;

    /**
     * The cached value of the '{@link #getMax_samples() <em>Max samples</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_samples()
     * @generated
     * @ordered
     */
    protected long max_samples = MAX_SAMPLES_EDEFAULT;

    /**
     * The default value of the '{@link #getMax_samples_per_instance() <em>Max samples per instance</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_samples_per_instance()
     * @generated
     * @ordered
     */
    protected static final long MAX_SAMPLES_PER_INSTANCE_EDEFAULT = 0L;

    /**
     * The cached value of the '{@link #getMax_samples_per_instance() <em>Max samples per instance</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_samples_per_instance()
     * @generated
     * @ordered
     */
    protected long max_samples_per_instance = MAX_SAMPLES_PER_INSTANCE_EDEFAULT;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected ResourceLimitsQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.RESOURCE_LIMITS_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public long getMax_instances() {
        return max_instances;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setMax_instances(long newMax_instances) {
        long oldMax_instances = max_instances;
        max_instances = newMax_instances;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES, oldMax_instances, max_instances));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public long getMax_samples() {
        return max_samples;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setMax_samples(long newMax_samples) {
        long oldMax_samples = max_samples;
        max_samples = newMax_samples;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES, oldMax_samples, max_samples));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public long getMax_samples_per_instance() {
        return max_samples_per_instance;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setMax_samples_per_instance(long newMax_samples_per_instance) {
        long oldMax_samples_per_instance = max_samples_per_instance;
        max_samples_per_instance = newMax_samples_per_instance;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE, oldMax_samples_per_instance,
                    max_samples_per_instance));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES:
                return getMax_instances();
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES:
                return getMax_samples();
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE:
                return getMax_samples_per_instance();
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
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES:
                setMax_instances((Long) newValue);
                return;
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES:
                setMax_samples((Long) newValue);
                return;
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE:
                setMax_samples_per_instance((Long) newValue);
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
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES:
                setMax_instances(MAX_INSTANCES_EDEFAULT);
                return;
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES:
                setMax_samples(MAX_SAMPLES_EDEFAULT);
                return;
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE:
                setMax_samples_per_instance(MAX_SAMPLES_PER_INSTANCE_EDEFAULT);
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
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES:
                return max_instances != MAX_INSTANCES_EDEFAULT;
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES:
                return max_samples != MAX_SAMPLES_EDEFAULT;
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE:
                return max_samples_per_instance != MAX_SAMPLES_PER_INSTANCE_EDEFAULT;
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
        result.append(" (max_instances: ");
        result.append(max_instances);
        result.append(", max_samples: ");
        result.append(max_samples);
        result.append(", max_samples_per_instance: ");
        result.append(max_samples_per_instance);
        result.append(')');
        return result.toString();
    }

} //ResourceLimitsQosPolicyImpl

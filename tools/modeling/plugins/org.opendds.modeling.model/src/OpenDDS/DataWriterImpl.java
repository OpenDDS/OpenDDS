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
 * <em><b>Data Writer</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.DataWriterImpl#getTopic <em>Topic</em>}</li>
 * <li>{@link OpenDDS.DataWriterImpl#getWriter_data_lifecycle <em>
 * Writer data lifecycle</em>}</li>
 * </ul>
 * </p>
 * 
 * @generated
 */
public class DataWriterImpl extends DataReaderWriterImpl implements DataWriter {
    /**
     * The cached value of the '{@link #getTopic() <em>Topic</em>}'
     * reference. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getTopic()
     * @generated
     * @ordered
     */
    protected Topic topic;

    /**
     * The cached value of the '{@link #getWriter_data_lifecycle()
     * <em>Writer data lifecycle</em>}' reference. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @see #getWriter_data_lifecycle()
     * @generated
     * @ordered
     */
    protected WriterDataLifecycleQosPolicy writer_data_lifecycle;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected DataWriterImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.DATA_WRITER;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Topic getTopic() {
        if (topic != null && topic.eIsProxy()) {
            InternalEObject oldTopic = (InternalEObject) topic;
            topic = (Topic) eResolveProxy(oldTopic);
            if (topic != oldTopic) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE, OpenDDSPackage.DATA_WRITER__TOPIC,
                            oldTopic, topic));
                }
            }
        }
        return topic;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Topic basicGetTopic() {
        return topic;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setTopic(Topic newTopic) {
        Topic oldTopic = topic;
        topic = newTopic;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_WRITER__TOPIC, oldTopic, topic));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public WriterDataLifecycleQosPolicy getWriter_data_lifecycle() {
        if (writer_data_lifecycle != null && writer_data_lifecycle.eIsProxy()) {
            InternalEObject oldWriter_data_lifecycle = (InternalEObject) writer_data_lifecycle;
            writer_data_lifecycle = (WriterDataLifecycleQosPolicy) eResolveProxy(oldWriter_data_lifecycle);
            if (writer_data_lifecycle != oldWriter_data_lifecycle) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DATA_WRITER__WRITER_DATA_LIFECYCLE, oldWriter_data_lifecycle,
                            writer_data_lifecycle));
                }
            }
        }
        return writer_data_lifecycle;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public WriterDataLifecycleQosPolicy basicGetWriter_data_lifecycle() {
        return writer_data_lifecycle;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setWriter_data_lifecycle(WriterDataLifecycleQosPolicy newWriter_data_lifecycle) {
        WriterDataLifecycleQosPolicy oldWriter_data_lifecycle = writer_data_lifecycle;
        writer_data_lifecycle = newWriter_data_lifecycle;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DATA_WRITER__WRITER_DATA_LIFECYCLE,
                    oldWriter_data_lifecycle, writer_data_lifecycle));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.DATA_WRITER__TOPIC:
                if (resolve) {
                    return getTopic();
                }
                return basicGetTopic();
            case OpenDDSPackage.DATA_WRITER__WRITER_DATA_LIFECYCLE:
                if (resolve) {
                    return getWriter_data_lifecycle();
                }
                return basicGetWriter_data_lifecycle();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case OpenDDSPackage.DATA_WRITER__TOPIC:
                setTopic((Topic) newValue);
                return;
            case OpenDDSPackage.DATA_WRITER__WRITER_DATA_LIFECYCLE:
                setWriter_data_lifecycle((WriterDataLifecycleQosPolicy) newValue);
                return;
        }
        super.eSet(featureID, newValue);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void eUnset(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.DATA_WRITER__TOPIC:
                setTopic((Topic) null);
                return;
            case OpenDDSPackage.DATA_WRITER__WRITER_DATA_LIFECYCLE:
                setWriter_data_lifecycle((WriterDataLifecycleQosPolicy) null);
                return;
        }
        super.eUnset(featureID);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public boolean eIsSet(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.DATA_WRITER__TOPIC:
                return topic != null;
            case OpenDDSPackage.DATA_WRITER__WRITER_DATA_LIFECYCLE:
                return writer_data_lifecycle != null;
        }
        return super.eIsSet(featureID);
    }

} // DataWriterImpl

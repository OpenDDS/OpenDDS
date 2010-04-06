/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import java.util.Collection;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.impl.ENotificationImpl;
import org.eclipse.emf.ecore.util.EObjectResolvingEList;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Application Target</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.ApplicationTargetImpl#getComponent_type <em>Component type</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTargetImpl#getLanguage <em>Language</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTargetImpl#getPlatform <em>Platform</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTargetImpl#getParticipants <em>Participants</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTargetImpl#getService_arguments <em>Service arguments</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class ApplicationTargetImpl extends EntityImpl implements ApplicationTarget {
    /**
     * The default value of the '{@link #getComponent_type() <em>Component type</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getComponent_type()
     * @generated
     * @ordered
     */
    protected static final ComponentType COMPONENT_TYPE_EDEFAULT = ComponentType.EXECUTABLE;

    /**
     * The cached value of the '{@link #getComponent_type() <em>Component type</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getComponent_type()
     * @generated
     * @ordered
     */
    protected ComponentType component_type = COMPONENT_TYPE_EDEFAULT;

    /**
     * The default value of the '{@link #getLanguage() <em>Language</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getLanguage()
     * @generated
     * @ordered
     */
    protected static final LanguageType LANGUAGE_EDEFAULT = LanguageType.CXX;

    /**
     * The cached value of the '{@link #getLanguage() <em>Language</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getLanguage()
     * @generated
     * @ordered
     */
    protected LanguageType language = LANGUAGE_EDEFAULT;

    /**
     * The default value of the '{@link #getPlatform() <em>Platform</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getPlatform()
     * @generated
     * @ordered
     */
    protected static final PlatformType PLATFORM_EDEFAULT = PlatformType.MPC_CDT;

    /**
     * The cached value of the '{@link #getPlatform() <em>Platform</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getPlatform()
     * @generated
     * @ordered
     */
    protected PlatformType platform = PLATFORM_EDEFAULT;

    /**
     * The cached value of the '{@link #getParticipants() <em>Participants</em>}' reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getParticipants()
     * @generated
     * @ordered
     */
    protected EList<DomainParticipant> participants;

    /**
     * The default value of the '{@link #getService_arguments() <em>Service arguments</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getService_arguments()
     * @generated
     * @ordered
     */
    protected static final String SERVICE_ARGUMENTS_EDEFAULT = null;

    /**
     * The cached value of the '{@link #getService_arguments() <em>Service arguments</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getService_arguments()
     * @generated
     * @ordered
     */
    protected String service_arguments = SERVICE_ARGUMENTS_EDEFAULT;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected ApplicationTargetImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.APPLICATION_TARGET;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ComponentType getComponent_type() {
        return component_type;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setComponent_type(ComponentType newComponent_type) {
        ComponentType oldComponent_type = component_type;
        component_type = newComponent_type == null ? COMPONENT_TYPE_EDEFAULT : newComponent_type;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.APPLICATION_TARGET__COMPONENT_TYPE,
                    oldComponent_type, component_type));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LanguageType getLanguage() {
        return language;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setLanguage(LanguageType newLanguage) {
        LanguageType oldLanguage = language;
        language = newLanguage == null ? LANGUAGE_EDEFAULT : newLanguage;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.APPLICATION_TARGET__LANGUAGE,
                    oldLanguage, language));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public PlatformType getPlatform() {
        return platform;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setPlatform(PlatformType newPlatform) {
        PlatformType oldPlatform = platform;
        platform = newPlatform == null ? PLATFORM_EDEFAULT : newPlatform;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.APPLICATION_TARGET__PLATFORM,
                    oldPlatform, platform));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EList<DomainParticipant> getParticipants() {
        if (participants == null) {
            participants = new EObjectResolvingEList<DomainParticipant>(DomainParticipant.class, this,
                    OpenDDSPackage.APPLICATION_TARGET__PARTICIPANTS);
        }
        return participants;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String getService_arguments() {
        return service_arguments;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setService_arguments(String newService_arguments) {
        String oldService_arguments = service_arguments;
        service_arguments = newService_arguments;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.APPLICATION_TARGET__SERVICE_ARGUMENTS,
                    oldService_arguments, service_arguments));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.APPLICATION_TARGET__COMPONENT_TYPE:
                return getComponent_type();
            case OpenDDSPackage.APPLICATION_TARGET__LANGUAGE:
                return getLanguage();
            case OpenDDSPackage.APPLICATION_TARGET__PLATFORM:
                return getPlatform();
            case OpenDDSPackage.APPLICATION_TARGET__PARTICIPANTS:
                return getParticipants();
            case OpenDDSPackage.APPLICATION_TARGET__SERVICE_ARGUMENTS:
                return getService_arguments();
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
            case OpenDDSPackage.APPLICATION_TARGET__COMPONENT_TYPE:
                setComponent_type((ComponentType) newValue);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__LANGUAGE:
                setLanguage((LanguageType) newValue);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__PLATFORM:
                setPlatform((PlatformType) newValue);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__PARTICIPANTS:
                getParticipants().clear();
                getParticipants().addAll((Collection<? extends DomainParticipant>) newValue);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__SERVICE_ARGUMENTS:
                setService_arguments((String) newValue);
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
            case OpenDDSPackage.APPLICATION_TARGET__COMPONENT_TYPE:
                setComponent_type(COMPONENT_TYPE_EDEFAULT);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__LANGUAGE:
                setLanguage(LANGUAGE_EDEFAULT);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__PLATFORM:
                setPlatform(PLATFORM_EDEFAULT);
                return;
            case OpenDDSPackage.APPLICATION_TARGET__PARTICIPANTS:
                getParticipants().clear();
                return;
            case OpenDDSPackage.APPLICATION_TARGET__SERVICE_ARGUMENTS:
                setService_arguments(SERVICE_ARGUMENTS_EDEFAULT);
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
            case OpenDDSPackage.APPLICATION_TARGET__COMPONENT_TYPE:
                return component_type != COMPONENT_TYPE_EDEFAULT;
            case OpenDDSPackage.APPLICATION_TARGET__LANGUAGE:
                return language != LANGUAGE_EDEFAULT;
            case OpenDDSPackage.APPLICATION_TARGET__PLATFORM:
                return platform != PLATFORM_EDEFAULT;
            case OpenDDSPackage.APPLICATION_TARGET__PARTICIPANTS:
                return participants != null && !participants.isEmpty();
            case OpenDDSPackage.APPLICATION_TARGET__SERVICE_ARGUMENTS:
                return SERVICE_ARGUMENTS_EDEFAULT == null ? service_arguments != null : !SERVICE_ARGUMENTS_EDEFAULT
                        .equals(service_arguments);
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
        result.append(" (component_type: ");
        result.append(component_type);
        result.append(", language: ");
        result.append(language);
        result.append(", platform: ");
        result.append(platform);
        result.append(", service_arguments: ");
        result.append(service_arguments);
        result.append(')');
        return result.toString();
    }

} //ApplicationTargetImpl

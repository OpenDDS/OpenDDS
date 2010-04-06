/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.util.EList;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Application Target</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.ApplicationTarget#getComponent_type <em>Component type</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTarget#getLanguage <em>Language</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTarget#getPlatform <em>Platform</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTarget#getParticipants <em>Participants</em>}</li>
 *   <li>{@link OpenDDS.ApplicationTarget#getService_arguments <em>Service arguments</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getApplicationTarget()
 * @model
 * @generated
 */
public interface ApplicationTarget extends Entity {
    /**
     * Returns the value of the '<em><b>Component type</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.ComponentType}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Component type</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Component type</em>' attribute.
     * @see OpenDDS.ComponentType
     * @see #setComponent_type(ComponentType)
     * @see OpenDDS.OpenDDSPackage#getApplicationTarget_Component_type()
     * @model required="true"
     * @generated
     */
    ComponentType getComponent_type();

    /**
     * Sets the value of the '{@link OpenDDS.ApplicationTarget#getComponent_type <em>Component type</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Component type</em>' attribute.
     * @see OpenDDS.ComponentType
     * @see #getComponent_type()
     * @generated
     */
    void setComponent_type(ComponentType value);

    /**
     * Returns the value of the '<em><b>Language</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.LanguageType}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Language</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Language</em>' attribute.
     * @see OpenDDS.LanguageType
     * @see #setLanguage(LanguageType)
     * @see OpenDDS.OpenDDSPackage#getApplicationTarget_Language()
     * @model required="true"
     * @generated
     */
    LanguageType getLanguage();

    /**
     * Sets the value of the '{@link OpenDDS.ApplicationTarget#getLanguage <em>Language</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Language</em>' attribute.
     * @see OpenDDS.LanguageType
     * @see #getLanguage()
     * @generated
     */
    void setLanguage(LanguageType value);

    /**
     * Returns the value of the '<em><b>Platform</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.PlatformType}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Platform</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Platform</em>' attribute.
     * @see OpenDDS.PlatformType
     * @see #setPlatform(PlatformType)
     * @see OpenDDS.OpenDDSPackage#getApplicationTarget_Platform()
     * @model required="true"
     * @generated
     */
    PlatformType getPlatform();

    /**
     * Sets the value of the '{@link OpenDDS.ApplicationTarget#getPlatform <em>Platform</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Platform</em>' attribute.
     * @see OpenDDS.PlatformType
     * @see #getPlatform()
     * @generated
     */
    void setPlatform(PlatformType value);

    /**
     * Returns the value of the '<em><b>Participants</b></em>' reference list.
     * The list contents are of type {@link OpenDDS.DomainParticipant}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Participants</em>' reference list isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Participants</em>' reference list.
     * @see OpenDDS.OpenDDSPackage#getApplicationTarget_Participants()
     * @model required="true"
     * @generated
     */
    EList<DomainParticipant> getParticipants();

    /**
     * Returns the value of the '<em><b>Service arguments</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Service arguments</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Service arguments</em>' attribute.
     * @see #setService_arguments(String)
     * @see OpenDDS.OpenDDSPackage#getApplicationTarget_Service_arguments()
     * @model
     * @generated
     */
    String getService_arguments();

    /**
     * Sets the value of the '{@link OpenDDS.ApplicationTarget#getService_arguments <em>Service arguments</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Service arguments</em>' attribute.
     * @see #getService_arguments()
     * @generated
     */
    void setService_arguments(String value);

} // ApplicationTarget

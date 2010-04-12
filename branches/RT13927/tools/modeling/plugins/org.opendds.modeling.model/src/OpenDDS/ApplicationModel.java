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
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Application Model</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.ApplicationModel#getApplications <em>
 * Applications</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getApplicationModel()
 * @model
 * @generated
 */
public interface ApplicationModel extends Model {
    /**
     * Returns the value of the '<em><b>Applications</b></em>'
     * containment reference list. The list contents are of type
     * {@link OpenDDS.ApplicationTarget}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Applications</em>' containment
     * reference list isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Applications</em>' containment
     *         reference list.
     * @see OpenDDS.OpenDDSPackage#getApplicationModel_Applications()
     * @model containment="true"
     * @generated
     */
    EList<ApplicationTarget> getApplications();

} // ApplicationModel

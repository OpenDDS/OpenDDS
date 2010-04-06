/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Content Filtered Topic</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.ContentFilteredTopic#getFilter_expression <em>Filter expression</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getContentFilteredTopic()
 * @model
 * @generated
 */
public interface ContentFilteredTopic extends TopicDescription {
    /**
     * Returns the value of the '<em><b>Filter expression</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Filter expression</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Filter expression</em>' attribute.
     * @see #setFilter_expression(String)
     * @see OpenDDS.OpenDDSPackage#getContentFilteredTopic_Filter_expression()
     * @model
     * @generated
     */
    String getFilter_expression();

    /**
     * Sets the value of the '{@link OpenDDS.ContentFilteredTopic#getFilter_expression <em>Filter expression</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Filter expression</em>' attribute.
     * @see #getFilter_expression()
     * @generated
     */
    void setFilter_expression(String value);

} // ContentFilteredTopic

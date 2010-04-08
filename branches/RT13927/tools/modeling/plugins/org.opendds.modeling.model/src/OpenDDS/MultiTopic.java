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
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Multi Topic</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.MultiTopic#getSubscription_expression <em>
 * Subscription expression</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getMultiTopic()
 * @model
 * @generated
 */
public interface MultiTopic extends TopicDescription {
    /**
     * Returns the value of the '
     * <em><b>Subscription expression</b></em>' attribute. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Subscription expression</em>'
     * attribute isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Subscription expression</em>'
     *         attribute.
     * @see #setSubscription_expression(String)
     * @see OpenDDS.OpenDDSPackage#getMultiTopic_Subscription_expression()
     * @model
     * @generated
     */
    String getSubscription_expression();

    /**
     * Sets the value of the '
     * {@link OpenDDS.MultiTopic#getSubscription_expression
     * <em>Subscription expression</em>}' attribute. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '
     *            <em>Subscription expression</em>' attribute.
     * @see #getSubscription_expression()
     * @generated
     */
    void setSubscription_expression(String value);

} // MultiTopic

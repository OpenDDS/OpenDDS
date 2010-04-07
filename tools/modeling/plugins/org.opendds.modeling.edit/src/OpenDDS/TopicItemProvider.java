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
import java.util.List;

import org.eclipse.emf.common.notify.AdapterFactory;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.edit.provider.ComposeableAdapterFactory;
import org.eclipse.emf.edit.provider.IEditingDomainItemProvider;
import org.eclipse.emf.edit.provider.IItemLabelProvider;
import org.eclipse.emf.edit.provider.IItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.IItemPropertySource;
import org.eclipse.emf.edit.provider.IStructuredItemContentProvider;
import org.eclipse.emf.edit.provider.ITreeItemContentProvider;

/**
 * This is the item provider adapter for a {@link OpenDDS.Topic}
 * object. <!-- begin-user-doc --> <!-- end-user-doc -->
 * 
 * @generated
 */
public class TopicItemProvider extends TopicDescriptionItemProvider implements IEditingDomainItemProvider,
        IStructuredItemContentProvider, ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
    /**
     * This constructs an instance from a factory and a notifier. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public TopicItemProvider(AdapterFactory adapterFactory) {
        super(adapterFactory);
    }

    /**
     * This returns the property descriptors for the adapted class.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public List<IItemPropertyDescriptor> getPropertyDescriptors(Object object) {
        if (itemPropertyDescriptors == null) {
            super.getPropertyDescriptors(object);

            addDurability_servicePropertyDescriptor(object);
            addTransport_priorityPropertyDescriptor(object);
            addTopic_dataPropertyDescriptor(object);
            addResource_limitsPropertyDescriptor(object);
            addReliabilityPropertyDescriptor(object);
            addOwnershipPropertyDescriptor(object);
            addLivelinessPropertyDescriptor(object);
            addHistoryPropertyDescriptor(object);
            addDurabilityPropertyDescriptor(object);
            addDestination_orderPropertyDescriptor(object);
            addDeadlinePropertyDescriptor(object);
            addLatency_budgetPropertyDescriptor(object);
        }
        return itemPropertyDescriptors;
    }

    /**
     * This adds a property descriptor for the Durability service
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDurability_servicePropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_durability_service_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_Topic_durability_service_feature",
                        "_UI_Topic_type"), OpenDDSPackage.Literals.TOPIC__DURABILITY_SERVICE, true, false, true, null,
                null, null));
    }

    /**
     * This adds a property descriptor for the Transport priority
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addTransport_priorityPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_transport_priority_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_Topic_transport_priority_feature",
                        "_UI_Topic_type"), OpenDDSPackage.Literals.TOPIC__TRANSPORT_PRIORITY, true, false, true, null,
                null, null));
    }

    /**
     * This adds a property descriptor for the Topic data feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addTopic_dataPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_topic_data_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_topic_data_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__TOPIC_DATA, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Resource limits
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addResource_limitsPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_resource_limits_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_Topic_resource_limits_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__RESOURCE_LIMITS, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Reliability feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addReliabilityPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_reliability_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_reliability_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__RELIABILITY, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Ownership feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addOwnershipPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_ownership_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_ownership_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__OWNERSHIP, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Liveliness feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addLivelinessPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_liveliness_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_liveliness_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__LIVELINESS, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the History feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addHistoryPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_history_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_history_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__HISTORY, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Durability feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDurabilityPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_durability_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_durability_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__DURABILITY, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Destination order
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDestination_orderPropertyDescriptor(Object object) {
        itemPropertyDescriptors
                .add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory).getRootAdapterFactory(),
                        getResourceLocator(), getString("_UI_Topic_destination_order_feature"), getString(
                                "_UI_PropertyDescriptor_description", "_UI_Topic_destination_order_feature",
                                "_UI_Topic_type"), OpenDDSPackage.Literals.TOPIC__DESTINATION_ORDER, true, false, true,
                        null, null, null));
    }

    /**
     * This adds a property descriptor for the Deadline feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDeadlinePropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_deadline_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_Topic_deadline_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__DEADLINE, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Latency budget feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addLatency_budgetPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_Topic_latency_budget_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_Topic_latency_budget_feature", "_UI_Topic_type"),
                OpenDDSPackage.Literals.TOPIC__LATENCY_BUDGET, true, false, true, null, null, null));
    }

    /**
     * This returns Topic.gif. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Object getImage(Object object) {
        return overlayImage(object, getResourceLocator().getImage("full/obj16/Topic"));
    }

    /**
     * This returns the label text for the adapted class. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public String getText(Object object) {
        String label = ((Topic) object).getName();
        return label == null || label.length() == 0 ? getString("_UI_Topic_type") : getString("_UI_Topic_type") + " "
                + label;
    }

    /**
     * This handles model notifications by calling
     * {@link #updateChildren} to update any cached children and by
     * creating a viewer notification, which it passes to
     * {@link #fireNotifyChanged}. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void notifyChanged(Notification notification) {
        updateChildren(notification);
        super.notifyChanged(notification);
    }

    /**
     * This adds {@link org.eclipse.emf.edit.command.CommandParameter}
     * s describing the children that can be created under this
     * object. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected void collectNewChildDescriptors(Collection<Object> newChildDescriptors, Object object) {
        super.collectNewChildDescriptors(newChildDescriptors, object);
    }

}

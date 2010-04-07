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
 * This is the item provider adapter for a
 * {@link OpenDDS.DataReaderWriter} object. <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * 
 * @generated
 */
public class DataReaderWriterItemProvider extends DomainEntityItemProvider implements IEditingDomainItemProvider,
        IStructuredItemContentProvider, ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
    /**
     * This constructs an instance from a factory and a notifier. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public DataReaderWriterItemProvider(AdapterFactory adapterFactory) {
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

            addDurabilityPropertyDescriptor(object);
            addDestination_orderPropertyDescriptor(object);
            addDeadlinePropertyDescriptor(object);
            addHistoryPropertyDescriptor(object);
            addUser_dataPropertyDescriptor(object);
            addResource_limitsPropertyDescriptor(object);
            addOwnershipPropertyDescriptor(object);
            addLivelinessPropertyDescriptor(object);
            addLatency_budgetPropertyDescriptor(object);
            addReliabilityPropertyDescriptor(object);
        }
        return itemPropertyDescriptors;
    }

    /**
     * This adds a property descriptor for the Durability feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDurabilityPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_durability_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_durability_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__DURABILITY, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Destination order
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDestination_orderPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_DataReaderWriter_destination_order_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_destination_order_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__DESTINATION_ORDER,
                true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Deadline feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDeadlinePropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_deadline_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_deadline_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__DEADLINE, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the History feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addHistoryPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_history_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_history_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__HISTORY, true, false,
                true, null, null, null));
    }

    /**
     * This adds a property descriptor for the User data feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addUser_dataPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_user_data_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_user_data_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__USER_DATA, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Resource limits
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addResource_limitsPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_DataReaderWriter_resource_limits_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_resource_limits_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__RESOURCE_LIMITS,
                true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Ownership feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addOwnershipPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_ownership_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_ownership_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__OWNERSHIP, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Liveliness feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addLivelinessPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_liveliness_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_liveliness_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__LIVELINESS, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Latency budget feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addLatency_budgetPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_DataReaderWriter_latency_budget_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_latency_budget_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__LATENCY_BUDGET, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Reliability feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addReliabilityPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReaderWriter_reliability_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReaderWriter_reliability_feature",
                        "_UI_DataReaderWriter_type"), OpenDDSPackage.Literals.DATA_READER_WRITER__RELIABILITY, true,
                false, true, null, null, null));
    }

    /**
     * This returns the label text for the adapted class. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public String getText(Object object) {
        String label = ((DataReaderWriter) object).getName();
        return label == null || label.length() == 0 ? getString("_UI_DataReaderWriter_type")
                : getString("_UI_DataReaderWriter_type") + " " + label;
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

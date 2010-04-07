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
import org.eclipse.emf.common.util.ResourceLocator;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.edit.provider.IEditingDomainItemProvider;
import org.eclipse.emf.edit.provider.IItemLabelProvider;
import org.eclipse.emf.edit.provider.IItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.IItemPropertySource;
import org.eclipse.emf.edit.provider.IStructuredItemContentProvider;
import org.eclipse.emf.edit.provider.ITreeItemContentProvider;
import org.eclipse.emf.edit.provider.ItemProviderAdapter;
import org.eclipse.emf.edit.provider.ViewerNotification;

import org.opendds.modeling.edit.EditPlugin;

/**
 * This is the item provider adapter for a {@link OpenDDS.Model}
 * object. <!-- begin-user-doc --> <!-- end-user-doc -->
 * 
 * @generated
 */
public class ModelItemProvider extends ItemProviderAdapter implements IEditingDomainItemProvider,
        IStructuredItemContentProvider, ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
    /**
     * This constructs an instance from a factory and a notifier. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public ModelItemProvider(AdapterFactory adapterFactory) {
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

        }
        return itemPropertyDescriptors;
    }

    /**
     * This specifies how to implement {@link #getChildren} and is
     * used to deduce an appropriate feature for an
     * {@link org.eclipse.emf.edit.command.AddCommand},
     * {@link org.eclipse.emf.edit.command.RemoveCommand} or
     * {@link org.eclipse.emf.edit.command.MoveCommand} in
     * {@link #createCommand}. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Collection<? extends EStructuralFeature> getChildrenFeatures(Object object) {
        if (childrenFeatures == null) {
            super.getChildrenFeatures(object);
            childrenFeatures.add(OpenDDSPackage.Literals.MODEL__ENTITIES);
        }
        return childrenFeatures;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EStructuralFeature getChildFeature(Object object, Object child) {
        // Check the type of the specified child object and return the
        // proper feature to use for
        // adding (see {@link AddCommand}) it as a child.

        return super.getChildFeature(object, child);
    }

    /**
     * This returns Model.gif. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Object getImage(Object object) {
        return overlayImage(object, getResourceLocator().getImage("full/obj16/Model"));
    }

    /**
     * This returns the label text for the adapted class. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public String getText(Object object) {
        return getString("_UI_Model_type");
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

        switch (notification.getFeatureID(Model.class)) {
            case OpenDDSPackage.MODEL__ENTITIES:
                fireNotifyChanged(new ViewerNotification(notification, notification.getNotifier(), true, false));
                return;
        }
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

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createContentFilteredTopic()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createMultiTopic()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createTopic()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createTopicStruct()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createDomain()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createDomainParticipant()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createDeadlineQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createDestinationOrderQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createDurabilityQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createDurabilityServiceQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createEntityFactoryQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createGroupDataQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createHistoryQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createLatencyBudgetQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createLifespanQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createLivelinessQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createOwnershipQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createOwnershipStrengthQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createPartitionQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createPresentationQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createReaderDataLifecycleQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createReliabilityQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createResourceLimitsQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createTimeBasedFilterQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createTopicDataQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createTransportPriorityQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createUserDataQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createWriterDataLifecycleQosPolicy()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createApplicationTarget()));

        newChildDescriptors.add(createChildParameter(OpenDDSPackage.Literals.MODEL__ENTITIES, OpenDDSFactory.eINSTANCE
                .createTransport()));
    }

    /**
     * Return the resource locator for this item provider's resources.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public ResourceLocator getResourceLocator() {
        return EditPlugin.INSTANCE;
    }

}

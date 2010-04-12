package org.opendds.modeling.diagram.application.part;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.gef.Tool;
import org.eclipse.gef.palette.PaletteContainer;
import org.eclipse.gef.palette.PaletteGroup;
import org.eclipse.gef.palette.PaletteRoot;
import org.eclipse.gef.palette.ToolEntry;
import org.eclipse.gmf.runtime.diagram.ui.tools.UnspecifiedTypeConnectionTool;
import org.eclipse.gmf.runtime.diagram.ui.tools.UnspecifiedTypeCreationTool;
import org.opendds.modeling.diagram.application.providers.OpenDDSElementTypes;

/**
 * @generated
 */
public class OpenDDSPaletteFactory {

	/**
	 * @generated
	 */
	public void fillPalette(PaletteRoot paletteRoot) {
		paletteRoot.add(createOpenDDS1Group());
	}

	/**
	 * Creates "OpenDDS" palette tool group
	 * 
	 * @generated
	 */
	private PaletteContainer createOpenDDS1Group() {
		PaletteGroup paletteContainer = new PaletteGroup(
				Messages.OpenDDS1Group_title);
		paletteContainer.setId("createOpenDDS1Group"); //$NON-NLS-1$
		paletteContainer.add(createDomainParticipant1CreationTool());
		paletteContainer.add(createDomainParticipantDomain2CreationTool());
		paletteContainer.add(createApplicationTarget3CreationTool());
		paletteContainer
				.add(createApplicationTargetParticipants4CreationTool());
		return paletteContainer;
	}

	/**
	 * @generated
	 */
	private ToolEntry createDomainParticipant1CreationTool() {
		List/* <IElementType> */types = new ArrayList/* <IElementType> */(1);
		types.add(OpenDDSElementTypes.ApplicationTarget_2001);
		NodeToolEntry entry = new NodeToolEntry(
				Messages.DomainParticipant1CreationTool_title,
				Messages.DomainParticipant1CreationTool_desc, types);
		entry.setId("createDomainParticipant1CreationTool"); //$NON-NLS-1$
		entry
				.setSmallIcon(OpenDDSElementTypes
						.getImageDescriptor(OpenDDSElementTypes.ApplicationTarget_2001));
		entry.setLargeIcon(entry.getSmallIcon());
		return entry;
	}

	/**
	 * @generated
	 */
	private ToolEntry createDomainParticipantDomain2CreationTool() {
		List/* <IElementType> */types = new ArrayList/* <IElementType> */(1);
		types.add(OpenDDSElementTypes.ApplicationTargetParticipants_4001);
		LinkToolEntry entry = new LinkToolEntry(
				Messages.DomainParticipantDomain2CreationTool_title,
				Messages.DomainParticipantDomain2CreationTool_desc, types);
		entry.setId("createDomainParticipantDomain2CreationTool"); //$NON-NLS-1$
		entry
				.setSmallIcon(OpenDDSElementTypes
						.getImageDescriptor(OpenDDSElementTypes.ApplicationTargetParticipants_4001));
		entry.setLargeIcon(entry.getSmallIcon());
		return entry;
	}

	/**
	 * @generated
	 */
	private ToolEntry createApplicationTarget3CreationTool() {
		ToolEntry entry = new ToolEntry(
				Messages.ApplicationTarget3CreationTool_title,
				Messages.ApplicationTarget3CreationTool_desc, null, null) {
		};
		entry.setId("createApplicationTarget3CreationTool"); //$NON-NLS-1$
		return entry;
	}

	/**
	 * @generated
	 */
	private ToolEntry createApplicationTargetParticipants4CreationTool() {
		ToolEntry entry = new ToolEntry(
				Messages.ApplicationTargetParticipants4CreationTool_title,
				Messages.ApplicationTargetParticipants4CreationTool_desc, null,
				null) {
		};
		entry.setId("createApplicationTargetParticipants4CreationTool"); //$NON-NLS-1$
		return entry;
	}

	/**
	 * @generated
	 */
	private static class NodeToolEntry extends ToolEntry {

		/**
		 * @generated
		 */
		private final List elementTypes;

		/**
		 * @generated
		 */
		private NodeToolEntry(String title, String description,
				List elementTypes) {
			super(title, description, null, null);
			this.elementTypes = elementTypes;
		}

		/**
		 * @generated
		 */
		@Override
		public Tool createTool() {
			Tool tool = new UnspecifiedTypeCreationTool(elementTypes);
			tool.setProperties(getToolProperties());
			return tool;
		}
	}

	/**
	 * @generated
	 */
	private static class LinkToolEntry extends ToolEntry {

		/**
		 * @generated
		 */
		private final List relationshipTypes;

		/**
		 * @generated
		 */
		private LinkToolEntry(String title, String description,
				List relationshipTypes) {
			super(title, description, null, null);
			this.relationshipTypes = relationshipTypes;
		}

		/**
		 * @generated
		 */
		@Override
		public Tool createTool() {
			Tool tool = new UnspecifiedTypeConnectionTool(relationshipTypes);
			tool.setProperties(getToolProperties());
			return tool;
		}
	}
}

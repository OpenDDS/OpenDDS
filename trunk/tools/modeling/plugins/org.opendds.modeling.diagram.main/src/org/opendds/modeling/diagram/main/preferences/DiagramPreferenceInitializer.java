/*
 * (c) Copyright Object Computing, Incorporated.  2005,2010.  All rights reserved.
 */
package org.opendds.modeling.diagram.main.preferences;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.jface.preference.IPreferenceStore;
import org.opendds.modeling.diagram.main.part.OpenDDSDiagramEditorPlugin;

/**
 * @generated
 */
public class DiagramPreferenceInitializer extends AbstractPreferenceInitializer {

	/**
	 * @generated NOT
	 */
	public void initializeDefaultPreferences() {
		IPreferenceStore store = getPreferenceStore();
		DiagramGeneralPreferencePage.initDefaults(store);
		DiagramAppearancePreferencePage.initDefaults(store);
		DiagramConnectionsPreferencePage.initDefaults(store);
		DiagramPrintingPreferencePage.initDefaults(store);
		DiagramRulersAndGridPreferencePage.initDefaults(store);

		// Custom code begin
		// GMFGen file allows diagram preferences to be defined that includes turning off pop-up bars.
		// However, with GMF 2.2.2 it was found that simply adding the preferences without changing any
		// values resulted in figures with black background.
		store.setDefault(
				org.eclipse.gmf.runtime.diagram.ui.preferences.IPreferenceConstants.PREF_SHOW_POPUP_BARS,
				false);
		// Custom code end
	}

	/**
	 * @generated
	 */
	protected IPreferenceStore getPreferenceStore() {
		return OpenDDSDiagramEditorPlugin.getInstance().getPreferenceStore();
	}
}

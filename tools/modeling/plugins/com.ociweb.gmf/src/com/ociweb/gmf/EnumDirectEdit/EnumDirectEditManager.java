/*
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/*
 * TODO: Submit this code to the Eclipe Foundation to use as part
 * of the resolution to bug "Support enum based labels" (https://bugs.eclipse.org/bugs/show_bug.cgi?id=158116).
 */

package com.ociweb.gmf.EnumDirectEdit;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.runtime.Assert;
import org.eclipse.draw2d.AncestorListener;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.emf.common.util.Enumerator;
import org.eclipse.gef.GraphicalEditPart;
import org.eclipse.gef.editparts.ZoomManager;
import org.eclipse.gef.tools.CellEditorLocator;
import org.eclipse.gef.tools.DirectEditManager;
import org.eclipse.gmf.runtime.common.core.util.Log;
import org.eclipse.gmf.runtime.common.core.util.Trace;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ITextAwareEditPart;
import org.eclipse.gmf.runtime.diagram.ui.internal.DiagramUIDebugOptions;
import org.eclipse.gmf.runtime.diagram.ui.internal.DiagramUIPlugin;
import org.eclipse.gmf.runtime.diagram.ui.internal.DiagramUIStatusCodes;
import org.eclipse.gmf.runtime.diagram.ui.label.ILabelDelegate;
import org.eclipse.gmf.runtime.diagram.ui.parts.DiagramGraphicalViewer;
import org.eclipse.gmf.runtime.draw2d.ui.figures.FigureUtilities;
import org.eclipse.gmf.runtime.draw2d.ui.figures.WrappingLabel;
import org.eclipse.gmf.runtime.draw2d.ui.mapmode.MapModeUtil;
import org.eclipse.jface.resource.DeviceResourceException;
import org.eclipse.jface.resource.FontDescriptor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.resource.ResourceManager;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Scrollable;
import org.eclipse.ui.part.CellEditorActionHandler;

/**
 * A Direct Edit Manager to allow the selection in a GMF figure for a EMF Enum Literal.
 * @see EnumDirectEditPolicy
 * @author melaasar (wrote TextDirectEditManager that this is primarily adapted from)
 * @author gmftools (adapted some from ComboBoxDirectEditManager found in <a href="http://code.google.com/p/gmftools/wiki/ComboBoxLabels">ComboBoxLabels</a>)
 * @author harrisb@ociweb.com (adapted to handle EMF Enums)
 */
@SuppressWarnings("restriction")
public class EnumDirectEditManager
    extends DirectEditManager {

    private String[] enumNames;

    private boolean committed = false;

    /**
     * flag used to avoid unhooking listeners twice if the UI thread is blocked
     */
    private boolean listenersAttached = true;

    /** String buffer to hold initial characters * */
    private StringBuffer initialString = new StringBuffer();

    /**
     * Cache the font descriptor when a font is created so that it can be
     * disposed later.
     */
    private List<FontDescriptor> cachedFontDescriptors = new ArrayList<FontDescriptor>();

    private CellEditorActionHandler actionHandler;

    private Font zoomLevelFont = null;

    /**
     * The superclass only relocates the cell editor when the location of the
     * editpart's figure moves, but we need to also relocate the cell editor
     * when the text figure's location changes.
     */
    private AncestorListener textFigureListener;

    protected static final String[] getEnumNames(Class<? extends Enum<?>> e)
    {
        String [] values = new String[e.getEnumConstants().length];
        int i = 0;
        for (Object obj : e.getEnumConstants()) {
        	Enumerator literal = (Enumerator) obj;
        	values[i++] = literal.getName();
        }
        return values;    	
    }
    /**
     * @param source
     * @param editorType
     * @param locator
     */
    public EnumDirectEditManager(GraphicalEditPart source, Class<?> editorType,
    		Class<? extends Enum<?>> e) {
        super(source, editorType, EnumDirectEditManager.getTextCellEditorLocator((ITextAwareEditPart)source));

        enumNames = getEnumNames(e);
    }

	public static CellEditorLocator getTextCellEditorLocator(
			final ITextAwareEditPart source) {

		final ILabelDelegate label = (ILabelDelegate) source
				.getAdapter(ILabelDelegate.class);
		if (label != null) {
			return new CellEditorLocator() {

				public void relocate(CellEditor celleditor) {
					Scrollable control = (Scrollable) celleditor.getControl();

					Rectangle rect = label.getTextBounds().getCopy();
					if (label.getText().length() <= 0) {
						// if there is no text, let's assume a default size
						// because it looks silly when the cell editor it tiny.
						rect.setSize(new Dimension(control.computeSize(
								SWT.DEFAULT, SWT.DEFAULT)));

						if (label.isTextWrapOn()) {
							// adjust the location of the cell editor based on
							// text
							// justification (i.e. where the cursor will be
							if (label.getTextJustification() == PositionConstants.RIGHT) {
								rect.translate(-rect.width, 0);
							} else if (label.getTextJustification() == PositionConstants.CENTER) {
								rect.translate(-rect.width / 2, 0);
							}
						}
					}

					if (label.isTextWrapOn()) {
						if (!control.getFont().isDisposed()) {
							// When zoomed in, the height of this rectangle is
							// not
							// sufficient because the text is shifted downwards
							// a
							// little bit. Add some to the height to compensate
							// for
							// this. I'm not sure why this is happening, but I
							// can
							// see the text shifting down even in a label on a
							// GEF
							// logic diagram when zoomed into 400%.
							int charHeight = FigureUtilities.getFontMetrics(
									control.getFont()).getHeight();
							rect.resize(0, charHeight / 2);
						}

					} else {

						rect.setSize(new Dimension(control.computeSize(
								SWT.DEFAULT, SWT.DEFAULT)));

						// If SWT.WRAP is not passed in as a style of the
						// TextCellEditor, then for some reason the first
						// character disappears upon entering the second
						// character. This should be investigated and an
						// SWT bug logged.
						int avr = FigureUtilities.getFontMetrics(
								control.getFont()).getAverageCharWidth();
						rect.setSize(new Dimension(control.computeSize(
								SWT.DEFAULT, SWT.DEFAULT)).expand(avr * 2, 0));
					}

					org.eclipse.swt.graphics.Rectangle newRect = control
							.computeTrim(rect.x, rect.y, rect.width,
									rect.height);
					if (!newRect.equals(control.getBounds())) {
						control.setBounds(newRect.x, newRect.y, newRect.width,
								newRect.height);
					}
				}
			};
		}

		// return a default figure locator
		return new CellEditorLocator() {
			public void relocate(CellEditor celleditor) {
				Control control = celleditor.getControl();
				Rectangle rect = source.getFigure().getBounds().getCopy();
				source.getFigure().translateToAbsolute(rect);
				if (!rect.equals(new Rectangle(control.getBounds()))) {
					control.setBounds(rect.x, rect.y, control.getSize().x,
							control.getSize().y);
				}
			}
		};
	}

    /**
     * @param source
     *            the <code>GraphicalEditPart</code> that is used to determine
     *            which <code>CellEditor</code> class to use.
     * @return the <code>Class</code> of the <code>CellEditor</code> to use
     *         for the text editing.
     */
    public static Class<ComboBoxCellEditor> getCellEditorClass() {
                
        return ComboBoxCellEditor.class;
    }

    /**
     * This method is overridden so that the editor class can have a style as
     * the style needs to be passed into the editor class when it is created. It
     * will default to the super behavior if an <code>editorType</code> was
     * passed into the constructor.
     * @since 2.1
     */
    protected CellEditor createCellEditorOn(Composite composite) {

        return new ComboBoxCellEditor(composite, enumNames, SWT.READ_ONLY | SWT.DROP_DOWN);
    }

    /**
     * Given a label figure object, this will calculate the 
     * correct Font needed to display into screen coordinates, taking into 
     * account the current mapmode.  This will typically be used by direct
     * edit cell editors that need to display independent of the zoom or any
     * coordinate mapping that is taking place on the drawing surface.
     * 
     * @param label the label to use for the font calculation
     * @return the <code>Font</code> that is scaled to the screen coordinates.
     * Note: the returned <code>Font</code> should not be disposed since it is
     * cached by a common resource manager.
     */
    protected Font getScaledFont(IFigure label) {
        Font scaledFont = label.getFont();
        FontData data = scaledFont.getFontData()[0];
        Dimension fontSize = new Dimension(0, MapModeUtil.getMapMode(label).DPtoLP(data.getHeight()));
        label.translateToAbsolute(fontSize);
        
        if( Math.abs( data.getHeight() - fontSize.height ) < 2 )
            fontSize.height = data.getHeight();

        try {
            FontDescriptor fontDescriptor = FontDescriptor.createFrom(data);
            cachedFontDescriptors.add(fontDescriptor);
            return getResourceManager().createFont(fontDescriptor);
        } catch (DeviceResourceException e) {
            Trace.catching(DiagramUIPlugin.getInstance(),
                DiagramUIDebugOptions.EXCEPTIONS_CATCHING, getClass(),
                "getScaledFont", e); //$NON-NLS-1$
            Log.error(DiagramUIPlugin.getInstance(),
                DiagramUIStatusCodes.IGNORED_EXCEPTION_WARNING, "getScaledFont", e); //$NON-NLS-1$
        }
        return JFaceResources.getDefaultFont();
    }

    
    protected void initCellEditor() {
        committed = false;

        ComboBoxCellEditor cellEditor = (ComboBoxCellEditor) getCellEditor();
        cellEditor.setItems(enumNames);

        ITextAwareEditPart textEP = (ITextAwareEditPart) getEditPart();

        IFigure label = textEP.getFigure();
        Assert.isNotNull(label);
        CCombo combo = (CCombo) cellEditor.getControl();
        // scale the font accordingly to the zoom level
        combo.setFont(getScaledFont(label));

        combo.pack();
       
    }

    /**
     * @see org.eclipse.gef.tools.DirectEditManager#commit()
     */
    protected void commit() {
        // content assist hacks
        if (committed) {
            bringDown();
            return;
        }
        committed = true;
		setDirty(true);
        super.commit();
    }

    /**
     * @see org.eclipse.gef.tools.DirectEditManager#bringDown()
     */
    protected void bringDown() {
        // myee - RATLC00523014: crashes when queued in asyncExec()
        eraseFeedback();
        
        initialString = new StringBuffer();
        
        Display.getCurrent().asyncExec(new Runnable() {

            public void run() {
                // Content Assist hack - allow proper cleanup on childen
                // controls
                EnumDirectEditManager.super.bringDown();
            }
        });
        
        for (Iterator<FontDescriptor> iter = cachedFontDescriptors.iterator(); iter.hasNext();) {
            getResourceManager().destroyFont(iter.next());           
        }
        cachedFontDescriptors.clear();
        
        if (actionHandler != null) {
            actionHandler.dispose();
            actionHandler = null;
        }
    }

    /**
     * This method is used to set the cell editors text
     * 
     * @param toEdit
     *            String to be set in the cell editor
     */
//    public void setEditText(String toEdit) {
//
//        // Get the cell editor
//        CellEditor cellEditor = getCellEditor();
//
//        // IF the cell editor doesn't exist yet...
//        if (cellEditor == null) {
//            // Do nothing
//            return;
//        }
//
//        // Get the Text Compartment Edit Part
//        ITextAwareEditPart textEP = (ITextAwareEditPart) getEditPart();
//
//        // Get the Text control
//        Text textControl = (Text) cellEditor.getControl();
//
//        // Set the Figures text
//        textEP.setLabelText(toEdit);
//        
//        
//        // See RATLC00522324
//        if (cellEditor instanceof TextCellEditorEx){
//            ((TextCellEditorEx)cellEditor).setValueAndProcessEditOccured(toEdit);
//        } else {
//            cellEditor.setValue(toEdit);
//        }
//        
//        // Set the controls text and position the caret at the end of the text
//        textControl.setSelection(toEdit.length());
//    }

    /**
     * Performs show and sets the edit string to be the initial character or string
     * @param initialChar
     */
    public void show(char initialChar) {
        initialString = initialString.append(initialChar);
        show();
//        if (SWT.getPlatform() != "carbon") { //$NON-NLS-1$ 
//            // Set the cell editor text to the initial character
//            setEditText(initialString.toString());
//        }
    }
    
    /**
     * This method obtains the fonts that are being used by the figure at its zoom level.
     * @param gep the associated <code>GraphicalEditPart</code> of the figure
     * @param actualFont font being used by the figure
     * @param display
     * @return <code>actualFont</code> if zoom level is 1.0 (or when there's an error),
     * new Font otherwise.
     */
    private Font getZoomLevelFont(Font actualFont, Display display) {
        Object zoom = getEditPart().getViewer().getProperty(ZoomManager.class.toString());
        
        if (zoom != null) {
            double zoomLevel = ((ZoomManager)zoom).getZoom();
            
            if (zoomLevel == 1.0f) 
                return actualFont;
            
            FontData[] fd = new FontData[actualFont.getFontData().length];
            FontData tempFD = null;
            
            for (int i=0; i < fd.length; i++) {
                tempFD = actualFont.getFontData()[i];
                
                fd[i] = new FontData(tempFD.getName(),(int)(zoomLevel * tempFD.getHeight()),tempFD.getStyle());
            }
            
            try {
                FontDescriptor fontDescriptor = FontDescriptor.createFrom(fd);
                cachedFontDescriptors.add(fontDescriptor);
                return getResourceManager().createFont(fontDescriptor);
            } catch (DeviceResourceException e) {
                Trace.catching(DiagramUIPlugin.getInstance(),
                    DiagramUIDebugOptions.EXCEPTIONS_CATCHING, getClass(),
                    "getZoomLevelFonts", e); //$NON-NLS-1$
                Log.error(DiagramUIPlugin.getInstance(),
                    DiagramUIStatusCodes.IGNORED_EXCEPTION_WARNING, "getZoomLevelFonts", e); //$NON-NLS-1$
                
                return actualFont;
            }
        }
        else
            return actualFont;
    }

    public void show() {
        super.show();

        IFigure fig = getEditPart().getFigure();

        Control control = getCellEditor().getControl();
        this.zoomLevelFont = getZoomLevelFont(fig.getFont(), control.getDisplay());

        control.setFont(this.zoomLevelFont);

        //since the font's have been resized, we need to resize the  Text control...
        getLocator().relocate(getCellEditor());

    }
    
    /**
     * 
     * Performs show and sends an extra mouse click to the point location so
     * that cursor appears at the mouse click point
     * 
     * The Text control does not allow for the cursor to appear at point location but
     * at a character location
     * 
     * @param location
     */
    public void show(Point location) {      
        show();
        sendClickToCellEditor(location);
    }

    private void sendClickToCellEditor(final Point location) {
        //make sure the diagram doesn't receive the click event..
        getCellEditor().getControl().setCapture(true);
        
        if (getCellEditor() != null && getCellEditor().getControl().getBounds().contains(location))
            sendMouseClick(location);
    }

    
    /**
     * 
     * Sends a SWT MouseUp and MouseDown event to the point location 
     * to the current Display
     * 
     * @param location
     */
    private void sendMouseClick(final Point location) {     
        
        final Display currDisplay = Display.getCurrent();
        currDisplay.asyncExec(new Runnable() {
	        public void run() {
	         Event event;
                event = new Event();
                event.type = SWT.MouseDown;
                event.button = 1;
                event.x = location.x;
                event.y = location.y;
                currDisplay.post(event);
                event.type = SWT.MouseUp;
                currDisplay.post(event);
	        }
	    });
    }

    protected void hookListeners() {
        super.hookListeners();

        // TODO: This gets around the problem of the cell editor not growing big
        // enough when in autosize mode because it doesn't listen to textflow
        // size changes. The superclass should be modified to not assume we want
        // to listen to the editpart's figure.
        ILabelDelegate label = (ILabelDelegate) getEditPart().getAdapter(
            ILabelDelegate.class);
        if (label != null && getEditPart().getFigure() instanceof WrappingLabel) {

            textFigureListener = new AncestorListener.Stub() {

                public void ancestorMoved(IFigure ancestor) {
                    getLocator().relocate(getCellEditor());
                }
            };
            ((IFigure) ((WrappingLabel) getEditPart().getFigure())
                .getTextFigure().getChildren().get(0))
                .addAncestorListener(textFigureListener);
        }
    }

    /*
     * Overrides super unhookListeners to set listeners attached flag This
     * method prevents unhooking listeners twice if the UI thread is blocked.
     * For example, a validation dialog may block the thread
     */
    protected void unhookListeners() {
        if (listenersAttached) {
            listenersAttached = false;
            super.unhookListeners();

            ILabelDelegate label = (ILabelDelegate) getEditPart().getAdapter(
                ILabelDelegate.class);
            if (label != null && textFigureListener != null) {
                ((IFigure) ((WrappingLabel) getEditPart().getFigure())
                    .getTextFigure().getChildren().get(0))
                    .removeAncestorListener(textFigureListener);
                textFigureListener = null;
            }
        }
    }

    /* 
     * Sets the listeners attached flag if the cell editor exists
     */
    protected void setCellEditor(CellEditor editor) {
        super.setCellEditor(editor);
        if (editor != null) {
            listenersAttached = true;
        }
    }

    public void showFeedback() {
        try {
            getEditPart().getRoot();
            super.showFeedback();
        } catch (Exception e) {
        	throw new RuntimeException(e);
            // TODO: handle exception
        }
        
    }    
    
    /**
     * Gets the resource manager to remember the resources allocated for this
     * graphical viewer. All resources will be disposed when the graphical
     * viewer is closed if they have not already been disposed.
     * @return
     */
    protected ResourceManager getResourceManager() {
        return ((DiagramGraphicalViewer) getEditPart().getViewer())
            .getResourceManager();
    }

}
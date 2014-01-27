package com.ociweb.gmf.figures;

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.GridData;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.gmf.runtime.diagram.ui.figures.ResizableCompartmentFigure;
import org.eclipse.swt.SWT;

/**
 * Provides a simple representation for a UML Package with a
 * small head on top of a body.
 * The graphical model definition's Figure definition that refers to this
 * custom figure must use Grid Layout. This is needed so that top node references
 * that can also be placed in packages can be dragged from the canvas (or
 * another package) onto any region in the package compartment and be
 * added to the compartment. Otherwise it appears that the element has
 * been added but was really just moved to another place on the canvas
 * that happens to overlap with the package.
 * @generated NOT
 */
public class UmlPackageFig extends Shape {


	private static final float HEAD_HEIGHT_RATIO = 0.20f;
	private static final float HEAD_WIDTH_RATIO = 0.40f;
	private static final Dimension MAX_HEAD_ROOM = new Dimension(40, 15);

	public UmlPackageFig() {
	}

	/**
	 * Since Grid Layout is used in the graphical definition model
	 * of UmlPackageFigure, need to provide Grid Layout data for
	 * its compartment.
	 */
	@Override
	public void add(IFigure figure, Object constraint, int index) {
		if (figure instanceof ResizableCompartmentFigure) {
			constraint = new GridData(SWT.FILL, SWT.FILL, true, true, 1, 1);
		}
		super.add(figure, constraint, index);
	}

	@Override
	protected void fillShape(Graphics graphics) {

		Rectangle head = getHeadRectangle(getBounds());
		graphics.fillRectangle(head);

		Rectangle body = getBodyRectangle(getBounds(), head);
		graphics.fillRectangle(body);
	}

	private static Rectangle getHeadRectangle(Rectangle figBounds) {
		int headWidth  = Math.min(MAX_HEAD_ROOM.width, Math.round(figBounds.width *  HEAD_WIDTH_RATIO));
		int headHeight = Math.min(MAX_HEAD_ROOM.height,  Math.round(figBounds.height * HEAD_HEIGHT_RATIO));
		Rectangle head = new Rectangle(figBounds.getLocation(), new Dimension(headWidth, headHeight));
		return head;
	}

	private static Rectangle getBodyRectangle(Rectangle figBounds, Rectangle head) {
		int bodyWidth = figBounds.width;
		int bodyHeight = figBounds.height - head.height;
		Point bodyTopLeftCorner = new Point(head.getTopLeft().x, head.getTopLeft().y + head.height);
		Rectangle body = new Rectangle(bodyTopLeftCorner, new Dimension(bodyWidth, bodyHeight));
		return body;
	}

	@Override
	protected void outlineShape(Graphics graphics) {
	    float lineInset = Math.max(1.0f, getLineWidthFloat()) / 2.0f;
	    int inset1 = (int)Math.floor(lineInset);
	    int inset2 = (int)Math.ceil(lineInset);

	    Rectangle r = Rectangle.SINGLETON.setBounds(getBounds());
	    r.x += inset1 ;
	    r.y += inset1;
	    r.width -= inset1 + inset2;
	    r.height -= inset1 + inset2;

		Rectangle head = getHeadRectangle(r);
		graphics.drawRectangle(head);

		Rectangle body = getBodyRectangle(r, head);
		graphics.drawRectangle(body);
	}

}

package com.ociweb.gmf.figures;

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;

/**
 * @generated NOT
 */
public class UmlPackageFig extends Shape {
	

	private static final float HEAD_HEIGHT_RATIO = 0.20f;
	private static final float HEAD_WIDTH_RATIO = 0.40f;

	public UmlPackageFig() {
	}
	
	@Override
	protected void fillShape(Graphics graphics) {
		
		Rectangle head = getHeadRectangle(getBounds());
		graphics.fillRectangle(head);
		
		Rectangle body = getBodyRectangle(getBounds(), head);
		graphics.fillRectangle(body);
	}

	private static Rectangle getHeadRectangle(Rectangle figBounds) {
		int headWidth = Math.round(figBounds.width * HEAD_WIDTH_RATIO);
		int headHeight = Math.round(figBounds.height * HEAD_HEIGHT_RATIO);
		Rectangle head = new Rectangle(figBounds.getLocation(), new Dimension(headWidth, headHeight));
		return head;
	}

	private static Rectangle getBodyRectangle(Rectangle figBounds, Rectangle head) {
		int bodyWidth = figBounds.width;
		int bodyHeight = Math.round(figBounds.height * (1.0f - HEAD_HEIGHT_RATIO));
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

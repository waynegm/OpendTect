#ifndef uirgbarraycanvas_h
#define uirgbarraycanvas_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uirgbarraycanvas.h,v 1.1 2007-02-09 13:41:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
#include "color.h"
class uiRGBArray;
class ioPixmap;


/*!\brief Provides a canvas where a uiRGBArray is pre-drawn.

  Handles borders, drawing and clearing.
  When the preDraw is triggered, the usedArea() is known, and the uiRGBArray
  was resized and cleared to the background color. You can choose to
  implement your own mkNewFill() function if you want to subclass, or catch
  the newFillNeeded notifier (if both: newFillNeeded is done first).
  At postDraw you typically put annotations and so forth, they are only
  needed in the updateArea().

 */

class uiRGBArrayCanvas : public uiCanvas
{
public:
    			uiRGBArrayCanvas(uiParent*,uiRGBArray&);

    void		setBorders(const uiSize& lfttop,const uiSize& rghtbot);
    void		setBGColor(const Color&); //!< everything
    void		setDrawArr(bool);	//!< Draw the arr or not?

    uiRect		arrArea() const		{ return arrarea_; }
    uiRGBArray&		rgbArray()		{ return rgbarr_; }
    const uiRGBArray&	rgbArray() const	{ return rgbarr_; }

    void		forceNewFill();

    Notifier<uiRGBArrayCanvas>	newFillNeeded;

    const uiRect&	updateArea()		{ return updarea_; }
    			//!< In this area the 'rest' needs to be drawn
    			//!< in your own reDrawHandler or at postDraw

    inline int		leftBorder() const	{ return border_.left(); }
    inline int		rightBorder() const	{ return border_.right(); }
    inline int		topBorder() const	{ return border_.top(); }
    inline int		bottomBorder() const	{ return border_.bottom(); }
    inline const Color&	bgColor() const		{ return bgcolor_; }
    inline bool		arrDrawn() const	{ return dodraw_; }

protected:

    uiRGBArray&		rgbarr_;
    uiRect		border_;
    Color		bgcolor_;
    bool		dodraw_;

    uiRect		arrarea_;
    uiRect		updarea_;
    ioPixmap*		pixmap_;

    virtual void	mkNewFill()		{}
    void		setupChg();
    void		beforeDraw(CallBacker*);
    void		reDrawHandler(uiRect);
    bool		createPixmap();

};


#endif

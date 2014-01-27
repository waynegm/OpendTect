#ifndef uiflatviewcontrol_h
#define uiflatviewcontrol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigroup.h"
#include "iopar.h"

class uiFlatViewer;
class uiFlatViewPropDlg;
class uiTabStackDlg;
class MouseEventHandler;
namespace FlatView { class ZoomMgr; class Viewer; }

/*!
\brief Tools to control uiFlatViewer(s).
*/

mExpClass(uiFlatView) uiFlatViewControl : public uiGroup
{
public:

			~uiFlatViewControl();

    void		addViewer(uiFlatViewer&);
    			//!< No attaching done. Viewer may be in other window.
    void		removeViewer(uiFlatViewer&);

    TypeSet<uiWorldRect>getBoundingBoxes() const;
    			//!< Returns bounding boxes of all viewers.
    virtual void	setNewView(Geom::Point2D<double>& mousepos,
				   Geom::Size2D<double>& newsize);
			/*!< Pass centre instead of mousepos if there is no
			MouseEvent. Retains uiWorldRect's LR/TB swapping while
			changing the input to the actual new values. */
    virtual void	flip(bool hor);
    			//!< reverses uiWorldRect's LR or TB swapping

    static bool		havePan(Geom::Point2D<double> oldcentre,
	    			Geom::Point2D<double> newcentre,
				Geom::Size2D<double> sz);
    static bool		haveZoom(Geom::Size2D<double> oldsz,
				 Geom::Size2D<double> newsz);

    void		setCategory( const char* cat )	{ category_ = cat; }
    			//!< Set the display pars defaults' `category`
   			//!< Needed if the main viewer uses no data pack
    const char*		category() const;
    			//!< If not set, returns data pack's category()

    Notifier<uiFlatViewControl>  infoChanged;	// CallBacker: CBCapsule<IOPar>
    Notifier<uiFlatViewControl>  viewerAdded;
    Notifier<uiFlatViewControl>  zoomChanged;

    uiRect			getViewRect(const uiFlatViewer*);
    static uiWorldRect		getZoomAndPanRect(Geom::Point2D<double>mousepos,
						  Geom::Size2D<double> newsz,
						  const uiWorldRect& view,
						  const uiWorldRect& bbox);
    static uiWorldRect		getZoomOrPanRect(Geom::Point2D<double> mousepos,
						 Geom::Size2D<double> newsz,
						 const uiWorldRect& view,
						 const uiWorldRect& bbox);
				/*!< If size of view and newsz differ,
				zooms in/out the uiWorldRect represented by
				view. Returns the resulting uiWorldRect after
				shifting it such that it falls inside the
				bounding box.*/
    uiWorldRect			getNewWorldRect(Geom::Point2D<double>& mousepos,
						Geom::Size2D<double>& newsz,
						const uiWorldRect& view,
						const uiWorldRect& bbox) const;
				/*!< Pass centre instead of mousepos if there
  				  is no MouseEvent.*/
    uiTabStackDlg*		propDialog();
    const FlatView::ZoomMgr&	zoomMgr() const { return zoommgr_; }

protected:

    			uiFlatViewControl(uiFlatViewer&,uiParent*,bool,bool);

    ObjectSet<uiFlatViewer> vwrs_;
    FlatView::ZoomMgr&	zoommgr_;
    bool		haverubber_;
    bool		withhanddrag_;
    BufferString	category_;
    IOPar		infopars_;

    uiFlatViewPropDlg*  propdlg_;

    MouseEventHandler&	mouseEventHandler(int vwridx,bool ofscene);
			//!< Returns MouseEventHandler of uiGraphicsScene if
			//!< ofscene is true else returns that of uiGraphicsView
    int			getViewerIdx(const MouseEventHandler*,bool ofscene);
			//!< ofscene should be true while passing
			//!< MouseEventHandler of uiGraphicsScene and false
			//!< while passing MouseEventHandler of uiGraphicsView.

    virtual void	finalPrepare()			{}
    virtual void	onFinalise(CallBacker*);
    virtual bool	canReUseZoomSettings( Geom::Point2D<double>,
	    				      Geom::Size2D<double> ) const;
    
    virtual void	dataChangeCB(CallBacker*);
    virtual void	rubBandCB(CallBacker*);
    virtual void	vwrAdded(CallBacker*)	{}
    virtual void	handDragStarted(CallBacker*)	{}
    virtual void	handDragging(CallBacker*)	{}
    virtual void	handDragged(CallBacker*)	{}
    virtual void	mouseMoveCB(CallBacker*);
    virtual void	keyPressCB(CallBacker*)		{}
    virtual void	usrClickCB(CallBacker*);
    virtual bool	handleUserClick(int vwridx)	{ return false; }
    
    virtual void	doPropertiesDialog(int vieweridx=0);
    virtual void	propDlgClosed(CallBacker*);
    virtual void	applyProperties(CallBacker* cb);
    virtual void	saveProperties(FlatView::Viewer&);

};

#endif


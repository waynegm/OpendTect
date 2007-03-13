#ifndef uiflatviewstdcontrol_h
#define uiflatviewstdcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: uiflatviewstdcontrol.h,v 1.3 2007-03-13 10:37:48 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "menuhandler.h"
class uiMenuHandler;
class uiToolButton;
class uiToolBar;


/*!\brief The standard tools to control uiFlatViewer(s). */

class uiFlatViewStdControl : public uiFlatViewControl
{
public:

    struct Setup
    {
			Setup( uiParent* p=0 )
			    : parent_(p)
			    , withstates_(true)		{}

	mDefSetupMemb(uiParent*,parent) //!< null => viewer's parent
	mDefSetupMemb(bool,	withstates)
    };

    			uiFlatViewStdControl(uiFlatViewer&,const Setup&);
    			~uiFlatViewStdControl();

protected:

    uiToolBar*		tb_;
    uiToolButton*	zoominbut_;
    uiToolButton*	zoomoutbut_;
    uiToolButton*	panupbut_;
    uiToolButton*	panleftbut_;
    uiToolButton*	panrightbut_;
    uiToolButton*	pandownbut_;
    uiToolButton*	manipbut_;
    uiToolButton*	drawbut_;
    uiToolButton*	parsbut_;

    virtual void	finalPrepare();
    void		updatePosButtonStates();

    void		vwChgCB(CallBacker*);
    void		zoomCB(CallBacker*);
    void		panCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		stateCB(CallBacker*);

    bool		handleUserClick();

    uiMenuHandler&      menu_;
    MenuItem           	propertiesmnuitem_;
    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);

};

#endif

#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.h,v 1.25 2002-08-14 08:45:25 arend Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

class uiMainWinBody;
class uiStatusBar;
class uiToolBar;
class uiMenuBar;
class uiObject;
class uiGroup;
class QWidget;
class uiDockWin;

class uiMainWin : public uiParent
{

public:
			/*!
			    nrStatusFlds == 0	: no statysbar
			    nrStatusFlds < 0	: creates empty statusbar.
				Add status fields yourself in that case.

			    if no parent, then modal is ignored
			     (always non-modal).
			*/
			uiMainWin( uiParent* parnt=0, 
				   const char* nm="uiMainWin",
				   int nrStatusFlds = 1, 
				   bool wantMenuBar = true,
				   bool wantToolBar = false,
				   bool modal=false );

    virtual		~uiMainWin();

    //! Dock Selector
    enum Dock
    {
            Top,        /*!< above the central uiGroup, below the menubar. */
            Bottom,     /*!< below the central uiGroup, above the status bar.*/
            Right,      /*!< to the right of the central uiGroup. */
            Left,       /*!< to the left of the central uiGroup.  */
            Minimized,  /*!< the toolbar is not shown - all handles of
                             minimized toolbars are drawn in one row below
                             the menu bar. */
	    TornOff,	/*!< the dock window floats as its own top level window
			     which always stays on top of the main window. */
	    Unmanaged	/*!< not managed by a uiMainWin */
    };

    uiStatusBar* 	statusBar();
    uiMenuBar* 		menuBar();
    uiToolBar* 		toolBar();
    uiToolBar* 		newToolBar(const char* nm="ToolBar");

    static uiMainWin*	activeWindow();

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(QWidget* mwimpl);

    void		setCaption( const char* txt );
    void		setIcon(const char* img[],const char* icntxt); //!< XPM
    void                show();
    void                close();
    void		toStatusBar(const char*, int fldidx=0 );

    uiObject*		uiObj();
    const uiObject*	uiObj() const;

    void		shallowRedraw( CallBacker* =0 )		{reDraw(false);}
    void		deepRedraw( CallBacker* =0 )		{reDraw(true); }
    void		reDraw(bool deep);
    uiGroup* 		topGroup();

    void		setShrinkAllowed( bool yn=true );
    bool		shrinkAllowed();

    bool		poppedUp() const;
    virtual uiMainWin*	mainwin()				{ return this; }

			//! automatically set by uiMain::setTopLevel
    void		setExitAppOnClose( bool yn );

    void		moveDockWindow( uiDockWin&, Dock d=Top );
    void		removeDockWindow(uiDockWin*);

    Notifier<uiMainWin>	finaliseStart;
    			//!< triggered when about to start finalising
    Notifier<uiMainWin>	finaliseDone;
    			//!< triggered when finalising finished

protected:

//    void		doPolish(CallBacker*);

			uiMainWin( const char* );

    uiMainWinBody*	body_;
};

#endif

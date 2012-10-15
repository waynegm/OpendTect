#ifndef uiwindowbase_h
#define uiwindowbase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigroup.h"

mClass(uiBase) uiWindowBase : public uiGroup
{
public:
    void		showMinMaxButtons();
    void		showAlwaysOnTop();
    
    void		setWindowTitle( const char* txt );

protected:
    
    static void		addWindow(uiWindowBase*);
    static void		removeWindow(uiWindowBase*);
    
    static Threads::Mutex		windowlistlock_;
    static ObjectSet<uiWindowBase>	windowlist_;

    mQtclass(QWidget)*		getWidget(int)	{ return getWindow(); }
				uiWindowBase(const char*);
    
    virtual mQtclass(QWidget)*	getWindow()		= 0;
};


#endif


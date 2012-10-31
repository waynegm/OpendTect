#ifndef uimain_h
#define uimain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/12/1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

mFDQtclass(QApplication);

mClass(uiBase) uiMain
{
public:
			uiMain(int& argc,char** argv);
			~uiMain();

    static uiMain&	theMain();
    static void		setXpmIconData( const char** xpmdata );
    
    void*		mainThread();

protected:
    static const char**		XpmIconData;

    static uiMain*		themain_;
    mQtclass(QApplication*)	app_;
};


mGlobal(uiBase) bool isMainThread(const void*);
mGlobal(uiBase) bool isMainThreadCurrent();

#endif


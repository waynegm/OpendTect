#ifndef uiosgfont_h
#define uiosgfont_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
 RCS:           $Id: ui3dviewer.h 26547 2012-10-01 14:02:41Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "vistext.h"

#include "uicoinmod.h"


mClass(uiCoin) uiOsgFontCreator : public visBase::OsgFontCreator
{
public:
    static void			initClass();
    osgText::Font*		createFont(const FontData&);
};

#endif


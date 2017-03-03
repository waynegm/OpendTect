#ifndef uistratmultidisplaywindow_h
#define uistratmultidisplaywindow_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Feb 2017
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uimainwin.h"
#include "datapack.h"

class uiFlatViewer;
class uiGroup;
class uiMultiFlatViewControl;
class uiStratLayerModel;

mExpClass(uiWellAttrib) uiStratMultiDisplayWin : public uiMainWin
{ mODTextTranslationClass(uiStratMultiDisplayWin);
public:
				uiStratMultiDisplayWin(
					uiParent*,DataPack::ID seisfdpid);

protected:
    uiGroup*			createSeisDisplay(DataPack::ID);
    uiGroup*			createSynthDisplay();
    uiGroup*			createLMDisplay();

    void			initViewer(uiFlatViewer*);

    uiFlatViewer*		seisvwr_;
    uiFlatViewer*		synthvwr_;
    uiStratLayerModel&		uislm_;
    uiGroup*			topgrp_;
    uiMultiFlatViewControl*	control_;
};

#endif

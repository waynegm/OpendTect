#ifndef uiprofilesynthseiscorrwin_h
#define uiprofilesynthseiscorrwin_h

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
class uiProfileSynthSeisCorrControl;
class uiStratLayerModel;
class ProfileModelBaseAuxDataMgr;
class ProfileModelFromEventData;

mExpClass(uiWellAttrib) uiProfileSynthSeisCorrWin : public uiMainWin
{ mODTextTranslationClass(uiProfileSynthSeisCorrWin);
public:
				uiProfileSynthSeisCorrWin(
				    uiParent*,const ProfileModelFromEventData&,
				    int nrseq);

protected:
    uiGroup*			createSeisDisplay();
    uiGroup*			createSynthDisplay();
    uiGroup*			createLMDisplay();

    void			initViewer(uiFlatViewer*);
    void			setSeisViewerZrg();

    uiFlatViewer*		seisvwr_;
    uiFlatViewer*		synthvwr_;
    uiStratLayerModel&		uislm_;
    uiGroup*			topgrp_;
    uiProfileSynthSeisCorrControl* control_;
    int				nrseq_;

    const ProfileModelFromEventData&	proffromevdata_;
    ProfileModelBaseAuxDataMgr*		seisprofadmgr_;
    ProfileModelBaseAuxDataMgr*		synthprofadmgr_;
    ProfileModelBaseAuxDataMgr*		laymodprofadmgr_;
};

#endif

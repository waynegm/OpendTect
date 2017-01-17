#ifndef uiprofilesetcreator_h
#define uiprofilesetcreator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		December 2016
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uigroup.h"
#include "datapack.h"

namespace FlatView { class AuxData; }
class uiGenInput;
class uiToolButton;
class uiListBox;
class uiFlatViewer;

class ProfileSet;
class ZValueProvider;

mExpClass(uiODMain) uiProfileSetCreatorGrp : public uiGroup
{ mODTextTranslationClass(uiProfileSetCreatorGrp);
public:
    mStruct(uiODMain) Data
    {
			Data( ProfileSet& p, const TypeSet<Coord>& linegeom )
			    : profs_(p)
			    , linegeom_(linegeom)
			    , seisfdpid_(DataPack::cNoID())	{}

	bool				is2d_;
	Pos::GeomID			geomid_;
	int				rdmlineid_;
	DataPack::ID			seisfdpid_;
	ProfileSet&			profs_;
	BufferStringSet			tiemarkernms_;
	ObjectSet<ZValueProvider>	zvalprovs_;
	const TypeSet<Coord>&		linegeom_;
	static const char*		dontUseStr()	{ return "<don't use>";}
    };
			uiProfileSetCreatorGrp(
			   uiParent*,const uiProfileSetCreatorGrp::Data&);
			~uiProfileSetCreatorGrp();
    int			nrProfs() const;
    virtual void	updateDisplay();
protected:
    uiGroup*				paramgrp_;
    uiGenInput*				nrprofsfld_;
    uiFlatViewer*			viewer_;
    uiListBox*				evlistbox_;
    uiToolButton*			addevbut_;
    uiToolButton*			rmevbut_;
    uiToolButton*			tiemarkerbut_;
    uiToolButton*			applybut_;
    uiProfileSetCreatorGrp::Data&	data_;

    ObjectSet<FlatView::AuxData>	horauxdatas_;

    void				addEventCB(CallBacker*);
    void				removeEventCB(CallBacker*);
    void				tieEventsCB(CallBacker*);
    void				createProfileSetCB(CallBacker*);
    virtual void			getEvents()			=0;
    void				drawEvents();
};


mExpClass(uiODMain) uiProfileSetCreatorGrpFactory
{
public:
    typedef uiProfileSetCreatorGrp*
	(*CreateFunc)(uiParent*,const uiProfileSetCreatorGrp::Data&);
    void				addCreateFunc(CreateFunc,const char*);
    uiProfileSetCreatorGrp*
	create(const char*,uiParent*,const uiProfileSetCreatorGrp::Data&);
    const BufferStringSet&	factoryNames() const	{ return keys_; }
protected:
    TypeSet<CreateFunc>		createfuncs_;
    BufferStringSet		keys_;
};


mGlobal(uiODMain) uiProfileSetCreatorGrpFactory& uiProfSetCRFac();

#endif

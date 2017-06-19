#ifndef profilemodelbaseauxdatamgr_h
#define profilemodelbaseauxdatamgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Feb 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "profilebase.h"
#include "profileview2model.h"
#include "profileviewpars.h"
#include "flatview.h"
class MouseEventHandler;
namespace Well { class Marker; }
class ProfileModelBase;


/*!\brief set properties for a certain unit */

mExpClass(Well) ProfileModelBaseAuxDataMgr : public CallBacker
{
public:
				ProfileModelBaseAuxDataMgr(
					const ProfileModelBase*,
					FlatView::Viewer&);
    virtual			~ProfileModelBaseAuxDataMgr();

    void			setModel(const ProfileModelBase* model)
				{ model_ = model; reset(); }
    ProfileView2Model&		view2Model()	{ return vw2mdl_; }
    const ProfileView2Model&	view2Model() const { return vw2mdl_; }
    ProfileViewPars&		drawPars()	{ return drawpars_; }
    const ProfileViewPars&	drawPars() const { return drawpars_; }

    void			reset();
    void			setIsFlattened(bool yn,const char* refmrknm);
    void			drawParsChanged(CallBacker* cb=0);
					//!< call after (a bunch of) change(s)

protected:

    mExpClass(Well) ProfileAux
    {
    public:
				ProfileAux(const ProfileBase&,
					   FlatView::Viewer&);
				~ProfileAux();

	FlatView::Viewer&	viewer_;
	FlatView::AuxData*	vert_;
	FlatView::AuxData*	wellnm_;
	ObjectSet<FlatView::AuxData> markers_;
	float			pos_;
	bool			iswell_;
    };


    FlatView::Viewer&		viewer_;
    const ProfileModelBase*	model_;
    ProfileView2Model		vw2mdl_;
    bool			isflattened_;
    BufferString		refmarkername_;
    ProfileViewPars		drawpars_;

    ObjectSet<FlatView::AuxData> markerconnections_;
    ManagedObjectSet<ProfileAux> profiles_;

    void			clearModel();
    void			addProfile(const ProfileBase&);
    void			addMarkerConnections();
    void			setVisibility();
    float			getViewZ(float depth,const ProfileBase&,
					 const Well::Marker*) const;

    void			updateAuxsCB(CallBacker*);
};

#endif

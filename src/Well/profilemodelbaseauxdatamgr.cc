/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : Feb 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilemodelbaseauxdatamgr.h"
#include "profilebase.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "wellmarker.h"
#include "wellman.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welltrack.h"
#include "flatposdata.h"
#include "polygon.h"
#include "ptrman.h"
#include "keystrs.h"


ProfileView2Model::ProfileView2Model( int nrseq )
    : zinft_(SI().depthsInFeet())
    , zrg_(0.5f,0.5f)
    , xrg_(0.5f,0.5f)
    , zindepth_(true)
{
    setNrSeq( nrseq );
}


void ProfileView2Model::setNrSeq( int nrseq )
{
    nrseq_ = nrseq;
}


Interval<float> ProfileView2Model::viewXRange() const
{
    return xrg_;
}


Interval<float> ProfileView2Model::viewZRange() const
{
    return Interval<float>( viewZ(zrg_.start), viewZ(zrg_.stop) );
}


float ProfileView2Model::modelX( float viewx, bool clp ) const
{
    float ret = nrseq_ > 0 ? (viewx-xrg_.start) / (float)(nrseq_) : viewx;
    if ( clp )
    {
	if ( ret < 0 ) ret = 0;
	if ( ret > 1 ) ret = 1;
    }
    return ret;
}


float ProfileView2Model::viewX( float modx, bool clp ) const
{
    float ret = nrseq_ <= 1 ? modx : xrg_.start + (xrg_.width()*modx);
    if ( clp )
    {
	if ( ret < xrg_.start ) ret = xrg_.start;
	if ( ret > xrg_.stop ) ret = xrg_.stop;
    }
    return ret;
}


float ProfileView2Model::modelZ( float viewz ) const
{
    return zindepth_ && zinft_ ? mFromFeetFactorF * viewz : viewz;
}


float ProfileView2Model::viewZ( float modelz ) const
{
    return zindepth_ && zinft_ ? mToFeetFactorF * modelz : modelz;
}


static const char* sKeyDrawWellNames = "Draw well names";
static const char* sKeyDrawMarkerNames = "Draw marker names";

#define mImplProfileViewParsIOP(fn) \
    iop.fn( "Depth skip", skipz_ ); \
    iop.fn( "Vertical display stacking", vertstack_ ); \
    iop.fn##YN( "Draw wells", drawwells_ ); \
    iop.fn##YN( sKeyDrawWellNames, drawwellnames_ ); \
    iop.fn##YN( "Draw control profiles", drawctrls_ ); \
    iop.fn##YN( "Draw markers", drawmarkers_ ); \
    iop.fn##YN( sKeyDrawMarkerNames, drawmarkernames_ ); \
    iop.fn##YN( "Draw marker connections", drawconnections_ ); \
    iop.fn##YN("Display marker names at control profile", drawctrlprofmrkrnms_);


void ProfileViewPars::usePar( const IOPar& iop )
{
    mImplProfileViewParsIOP( get );
    if ( !drawwells_ )
	drawwellnames_ = false;
    if ( !drawmarkers_ )
    {
	drawmarkernames_ = false;
	drawctrlprofmrkrnms_ = false;
    }
}


void ProfileViewPars::fillPar( IOPar& iop ) const
{
    mImplProfileViewParsIOP( set );
    if ( !drawwells_ )
	iop.removeWithKey( sKeyDrawWellNames );
    if ( !drawmarkers_ )
	iop.removeWithKey( sKeyDrawMarkerNames );
}


bool ProfileViewPars::operator ==( const ProfileViewPars& oth ) const
{
    if ( drawwells_ != oth.drawwells_
	|| drawctrls_ != oth.drawctrls_
	|| drawmarkers_ != oth.drawmarkers_
	|| drawconnections_ != oth.drawconnections_
	|| !mIsEqual(skipz_,oth.skipz_,1e-6)
	|| drawmarkernames_ != oth.drawmarkernames_
	|| drawctrlprofmrkrnms_ != oth.drawctrlprofmrkrnms_
	|| !(selmrkrnms_ == oth.selmrkrnms_) )
	return false;
    //TODO LayerModelPropDispPars?
    return drawwells_ ? drawwellnames_ == oth.drawwellnames_ : true;
}


ProfileModelBaseAuxDataMgr::ProfileAux::ProfileAux( const ProfileBase& p,
							  FlatView::Viewer& vw )
    : viewer_(vw)
    , vert_(0)
    , wellnm_(0)
    , pos_(p.pos_)
    , iswell_(p.isPrimary())
{
}


ProfileModelBaseAuxDataMgr::ProfileAux::~ProfileAux()
{
    delete viewer_.removeAuxData( vert_ );
    delete viewer_.removeAuxData( wellnm_ );
    viewer_.removeAuxDatas( markers_ );
}


#define mCrAuxData(varnm,txt,act) \
    varnm = viewer_.createAuxData( txt ); \
    if ( !varnm ) act; \
    varnm->zvalue_ = 1000; \
    varnm->cansetcursor_ = false; \
    varnm->linestyle_.type_ = LineStyle::Solid; \
    viewer_.addAuxData( varnm )


ProfileModelBaseAuxDataMgr::ProfileModelBaseAuxDataMgr(
		const ProfileModelBase* pfm, FlatView::Viewer& vwr  )
    : model_(pfm)
    , viewer_(vwr)
    , isflattened_(false)
    , refmarkername_("____") // make sure it's 'dirty' the first time
{
    reset();
}


ProfileModelBaseAuxDataMgr::~ProfileModelBaseAuxDataMgr()
{
    clearModel();
}


void ProfileModelBaseAuxDataMgr::clearModel()
{
    profiles_.erase();
    viewer_.removeAuxDatas( markerconnections_ );
}


void ProfileModelBaseAuxDataMgr::setIsFlattened( bool yn, const char* mrkr )
{
    const bool needreset = isflattened_ != yn || refmarkername_ != mrkr;
    isflattened_ = yn; refmarkername_ = mrkr;
    if ( needreset )
	reset();
}


void ProfileModelBaseAuxDataMgr::reset()
{
    clearModel();
    if ( !model_->isEmpty() )
    {
	for ( int idx=0; idx<model_->size(); idx ++ )
	    addProfile( *model_->profs_[idx] );
	addMarkerConnections();
    }

    setVisibility();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void ProfileModelBaseAuxDataMgr::drawParsChanged( CallBacker* )
{
    reset();
}


void ProfileModelBaseAuxDataMgr::setVisibility()
{
    for ( int idx=0; idx<profiles_.size(); idx++ )
    {
	ProfileAux& profaux = *profiles_[idx];
	const bool ison = (profaux.iswell_ && drawpars_.drawwells_)
			|| (!profaux.iswell_ && drawpars_.drawctrls_);
	if ( profaux.vert_ )
	    profaux.vert_->enabled_ = ison;
	if ( profaux.wellnm_ )
	    profaux.wellnm_->enabled_ = ison && drawpars_.drawwellnames_;
	for ( int idaux=0; idaux<profaux.markers_.size(); idaux++ )
	{
	    profaux.markers_[idaux]->enabled_ = ison && drawpars_.drawmarkers_;
	    const bool& drawmarkernms = drawpars_.drawmarkers_ &&
		(profaux.iswell_ ? drawpars_.drawmarkernames_
				 : drawpars_.drawctrlprofmrkrnms_);
	    profaux.markers_[idaux]->namepos_ = !drawmarkernms ? mUdf(int) : 0;
	}
    }

    for ( int idx=0; idx<markerconnections_.size(); idx++ )
	markerconnections_[idx]->enabled_ = drawpars_.drawconnections_;
}


#define mGetWellData( wd, welld2t, wellid ) \
const Well::Data* wd = Well::MGR().get( wellid ); \
if ( !wd ) \
{ \
    pErrMsg( "Well not read properly" ); \
    return mUdf(float); \
} \
const Well::D2TModel* welld2t = wd->d2TModel();


float ProfileModelBaseAuxDataMgr::getViewZ( float z, const ProfileBase& prof,
					    const Well::Marker* refmrk ) const
{
    if ( mIsUdf(z) )
	return mUdf(float);

    if ( refmrk )
       z -= refmrk->dah();
    if ( !vw2mdl_.zInDepth() )
    {
	if ( model_->zTransfrom() && !prof.coord_.isUdf() )
	{
	    TrcKey proftk;
	    proftk.setFrom( prof.coord_ );
	    return model_->zTransfrom()->transformTrcBack( proftk, z );
	}

	MultiID wellid;
	if ( prof.wellid_.isUdf() )
	{
	    const int wellprof1idx = model_->wellIndexBefore( prof.pos_ );
	    const int wellprof2idx = model_->wellIndexAfter( prof.pos_ );
	    if ( wellprof1idx<0 && wellprof2idx<0 )
	    {
		pErrMsg( "Huh! No wellprofile in model." );
		return mUdf(float);
	    }

	    if ( wellprof2idx< 0 )
		wellid = model_->profs_[wellprof1idx]->wellid_;
	    else if ( wellprof1idx < 0 )
		wellid = model_->profs_[wellprof2idx]->wellid_;
	    else
	    {
		const ProfileBase& prof1 = *model_->profs_[wellprof1idx];
		const ProfileBase& prof2 = *model_->profs_[wellprof2idx];
		const float profrelpos =
		    (prof.pos_ - prof1.pos_) / (prof2.pos_-prof1.pos_);
		mGetWellData( wd1, well1d2t, prof1.wellid_ );
		mGetWellData( wd2, well2d2t, prof2.wellid_ );
		const float w1dah = wd1->track().getDahForTVD( z );
		const float w2dah = wd2->track().getDahForTVD( z );
		//because wellmarker dah in profiles are actually tvds
		if ( mIsUdf(w1dah) || mIsUdf(w2dah) )
		    return mUdf(float);

		const float well1z = well1d2t->getTime( w1dah, wd1->track() );
		const float well2z = well2d2t->getTime( w2dah, wd2->track() );
		return well1z*(1-profrelpos) + well2z * profrelpos;
	    }
	}
	else
	    wellid = prof.wellid_;

	mGetWellData( wd, welld2t, wellid );
	const float wdah = wd->track().getDahForTVD( z );
	z = welld2t->getTime( wdah, wd->track() );
    }

    return vw2mdl_.viewZ( z );	  
}


void ProfileModelBaseAuxDataMgr::addProfile( const ProfileBase& prof )
{
    const Well::Marker* refmrk = !isflattened_ ? 0
				: prof.markers_.getByName( refmarkername_ );
    if ( isflattened_ && !refmrk )
	return;

    if ( (prof.isPrimary() && !drawpars_.drawwells_) ||
	 (prof.isCtrl() && !drawpars_.drawctrls_)  )
	return;

    ProfileAux* pfaux = new ProfileAux( prof, viewer_ );
    profiles_ += pfaux;

    StepInterval<float> yrg( vw2mdl_.viewZRange() );
    if ( refmrk )
	yrg.shift( -getViewZ(refmrk->dah(),prof,refmrk) );
    const float xpos = vw2mdl_.viewX( prof.pos_ );

    mCrAuxData( pfaux->vert_, 0 , return );
    pfaux->vert_->poly_ += FlatView::Point( xpos, yrg.start );
    pfaux->vert_->poly_ += FlatView::Point( xpos, yrg.stop );
    pfaux->vert_->linestyle_.color_ = Color::Black();
    pfaux->vert_->linestyle_.width_ = pfaux->iswell_ ? 3 : 2;
    if ( !pfaux->iswell_ )
	pfaux->vert_->linestyle_.type_ = LineStyle::Dot;

    if ( pfaux->iswell_ )
    {
	mDynamicCastGet(const ProfileBase*,profile,&prof);
	mCrAuxData( pfaux->wellnm_, profile->name(), return );
	pfaux->wellnm_->poly_ += FlatView::Point( xpos, yrg.stop );
	pfaux->wellnm_->poly_ += FlatView::Point( xpos, yrg.stop-0.1 );
	pfaux->wellnm_->linestyle_ = LineStyle( LineStyle::Solid, 1,
						Color(0,0,100) );
	pfaux->wellnm_->namealignment_ = mAlignment(HCenter,Bottom);
	pfaux->wellnm_->namepos_ = 1; // after last
	pfaux->wellnm_->zvalue_ = 1000;
    }

    for ( int idx=0; idx<drawpars_.selmrkrnms_.size(); idx++ )
    {
	const Well::Marker* wm =
	    prof.markers_.getByName( drawpars_.selmrkrnms_.get(idx) );
	if ( !wm )
	    continue;

	FlatView::AuxData* mCrAuxData( ad, wm->name(), continue );
	ad->poly_ += FlatView::Point( xpos, getViewZ(wm->dah(),prof,refmrk) );
	ad->zvalue_ = 1000;
	ad->markerstyles_ += MarkerStyle2D(MarkerStyle2D::Plane,4,wm->color());
	ad->namealignment_ = mAlignment(Left,Bottom);
	ad->namepos_ = 0;
	pfaux->markers_ += ad;
    }
}


void ProfileModelBaseAuxDataMgr::addMarkerConnections()
{
    if ( profiles_.size() < 2 || !drawpars_.drawconnections_ )
	return;

    for ( int idx=0; idx<model_->size()-1; idx ++ )
    {
	const ProfileBase& prof1 = *model_->profs_[idx];
	const ProfileBase& prof2 = *model_->profs_[idx+1];
	const float xpos1 = vw2mdl_.viewX( prof1.pos_ );
	const float xpos2 = vw2mdl_.viewX( prof2.pos_ );
	const Well::Marker* refmrk1 = isflattened_
				? prof1.markers_.getByName(refmarkername_) : 0;
	const Well::Marker* refmrk2 = isflattened_
				? prof2.markers_.getByName(refmarkername_) : 0;
	if ( isflattened_ && (!refmrk1 || !refmrk2) )
	    continue;

	for ( int idmrk=0; idmrk<drawpars_.selmrkrnms_.size(); idmrk++ )
	{
	    FixedString markernm( drawpars_.selmrkrnms_.get(idmrk) );
	    const Well::Marker* mrk1 = prof1.markers_.getByName( markernm );
	    if ( !mrk1 ) continue;

	    const Well::Marker* mrk2 = prof2.markers_.getByName( markernm );
	    if ( !mrk2 ) continue;

	    BufferString adnm( markernm, " " );
	    adnm.add( idx ).add( "-" ).add( idx+1 );
	    FlatView::AuxData* mCrAuxData( ad, adnm, continue );
	    ad->poly_ +=
		FlatView::Point( xpos1, getViewZ(mrk1->dah(),prof1,refmrk1) );
	    ad->poly_ +=
		FlatView::Point( xpos2, getViewZ(mrk2->dah(),prof2,refmrk2) );
	    ad->linestyle_ = LineStyle( LineStyle::Dot, 2, mrk1->color() );

	    markerconnections_ += ad;
	}
    }
}

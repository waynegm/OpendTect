/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uichecklist.cc,v 1.1 2011-09-06 12:02:43 cvsbert Exp $";

#include "uichecklist.h"
#include "uibutton.h"
#include "bufstringset.h"


uiCheckList::uiCheckList( uiParent* p, const char* t1, const char* t2,
			  uiCheckList::Pol pl )
    : uiGroup(p,"CheckList")
    , pol_(pl)
    , changed(this)
{
    addBox( t1 );
    addBox( t2 );
    finaliseDone.notify( mCB(this,uiCheckList,initGrp) );
}


uiCheckList::uiCheckList( uiParent* p, const BufferStringSet& bss,
			  uiCheckList::Pol pl )
    : uiGroup(p,"CheckList")
    , pol_(pl)
    , changed(this)
{
    for ( int idx=0; idx<bss.size(); idx++ )
	addBox( bss.get(idx), false );
}


void uiCheckList::addBox( const char* txt, bool hor )
{
    uiCheckBox* cb = new uiCheckBox( this, txt );
    if ( !boxs_.isEmpty() )
	cb->attach( hor ? rightOf : alignedBelow, boxs_[ boxs_.size()-1 ] );
    boxs_ += cb;
}


void uiCheckList::initGrp( CallBacker* )
{
    const CallBack cb( mCB(this,uiCheckList,boxChk) );
    for ( int idx=0; idx<boxs_.size(); idx++ )
	boxs_[idx]->activated.notify( cb );
}


bool uiCheckList::isChecked( int idx ) const
{
    return idx < 0 || idx >= boxs_.size() ? false : boxs_[idx]->isChecked();
}


void uiCheckList::setChecked( int idx, bool yn )
{
    if ( idx >= 0 && idx < boxs_.size() )
	boxs_[idx]->setChecked( yn );
}


void uiCheckList::boxChk( CallBacker* c )
{
    mDynamicCastGet(uiCheckBox*,cb,c)
    if ( !cb ) { pErrMsg("Huh"); return; }
    clicked_ = cb;

    switch ( pol_ )
    {
    case Unrel:		break;
    case NotAll:	ensureOne(false); break;
    case AtLeastOne:	ensureOne(true); break;
    default:		handleChain(); break;
    }

    changed.trigger();
}


void uiCheckList::ensureOne( bool ischckd )
{
    if ( clicked_->isChecked() == ischckd )
	return;

    int boxidx = 0;
    for ( int idx=0; idx<boxs_.size(); idx++ )
    {
	if ( boxs_[idx]->isChecked() == ischckd )
	    return;
	else if ( boxs_[idx] == clicked_ )
	    boxidx = idx;
    }
    int box2check = boxidx - 1;
    if ( box2check < 0 ) box2check = boxs_.size() - 1;
    if ( box2check == boxidx ) return; // single button
    setBox( box2check, ischckd );
}


void uiCheckList::handleChain()
{
    pErrMsg( "TODO: implement uiCheckList::handleChain()" );
}


void uiCheckList::setBox( int idx, bool chkd, bool shw )
{
    uiCheckBox* cb = boxs_[idx];
    NotifyStopper ns( cb->activated );
    cb->setChecked( chkd );
    cb->display( shw );
}

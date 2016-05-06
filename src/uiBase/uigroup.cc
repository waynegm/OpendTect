/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/

#include "uigroup.h"
#include "envvars.h"

#include <QFrame>


mUseQtnamespace


uiGroup::uiGroup( uiParent* parnt, const char* nm, bool manage )
    : uiParent( nm )
    , uiObject( parnt, nm )
    , layoutmgr_( new uiLayoutMgr(this) )
{
}


uiGroup::~uiGroup()
{
    delete layoutmgr_;
}


const uiObject* uiGroup::getHAlignObj() const
{
    return layoutmgr_->getHAlignObj();
}


void uiGroup::setHAlignObj( const uiObject* obj )
{
    layoutmgr_->setHAlignObj( obj );
}


const uiObject* uiGroup::getHCenterObj() const
{
    return layoutmgr_->getHCenterObj();
}


void uiGroup::setHCenterObj( const uiObject* obj )
{
    layoutmgr_->setHCenterObj( obj );
}


void uiGroup::finalise()
{
    preFinalise().trigger();
    
    for ( int idx=0; idx<layoutmgr_->nrChildren(); idx++ )
    {
        layoutmgr_->getChild(idx)->finalise();
    }
    
    uiObject::finalise();
    
    getLayoutMgr()->populateGrid();
    
    postFinalise().trigger();
}


int uiGroup::getNrWidgets() const
{
    return layoutmgr_->nrChildren();
}


QWidget* uiGroup::getWidget(int idx)
{
    uiObject* obj = layoutmgr_->getChild(idx);
    return obj && obj->getNrWidgets() ? obj->getWidget(0) : 0;
}


void uiGroup::translateText()
{
    if ( !childList() )
        return;
    
    for ( int idx=0; idx<childList()->size(); idx++ )
    {
        uiObject* child = const_cast<uiObject*>((*childList())[idx]);
        child->translateText();
    }
}


void uiGroup::setSensitive( bool yn )
{
    const ObjectSet<uiObject>* childlist = childList();
    if ( !childlist ) return;

    for ( int idx=0; idx<childlist->size(); idx++ )
	((uiObject*)(*childlist)[idx])->setSensitive( yn );
}


void uiGroup::setSize( const uiSize& sz )
{
    /*
    if ( sz.width() <= 0 || sz.height() <= 0 )
	return;
    const int oldwidth = mainObject()->width();
    const int oldheight = mainObject()->height();
    if ( !oldwidth || !oldheight )
	return;

    const float wfac = sz.width()/(float)oldwidth;
    const float hfac = sz.height()/(float)oldheight;

    reSizeChildren( mainObject(), wfac, hfac );
    const int newwdth = mNINT32(oldwidth*wfac);
    const int newhght = mNINT32(oldheight*hfac);
    mainObject()->setMinimumWidth( newwdth );
    mainObject()->setMinimumHeight( newhght );
    mainObject()->setMaximumWidth( newwdth );
    mainObject()->setMaximumHeight( newhght );
    */
}


void uiGroup::reSizeChildren( const uiObject* obj, float wfac, float hfac )
{
    /*
    if ( wfac <= 0 || hfac <= 0 || !obj->childList() )
	return;

    for ( int idchild=0; idchild<obj->childList()->size(); idchild++ )
    {
	const uiBaseObject* child = (*obj->childList())[idchild];
	mDynamicCastGet(const uiObject*,objchild,child)
	if ( objchild )
	    reSizeChildren( objchild, wfac, hfac );

	const int newwdth = mNINT32(objchild->width()*wfac);
	const int newhght = mNINT32(objchild->height()*hfac);
	((uiObject*)(objchild))->setMinimumWidth( newwdth );
	((uiObject*)(objchild))->setMaximumWidth( newwdth );
	((uiObject*)(objchild))->setMinimumHeight( newhght );
	((uiObject*)(objchild))->setMaximumHeight( newhght );
    }
    */
}


uiLayoutGroup::uiLayoutGroup( uiParent* p, const char* nm )
    : uiGroup( p, nm )
    , widget_( new QFrame )
{}


QWidget* uiLayoutGroup::getWidget(int idx)
{ return idx ? 0 : widget_; }

void uiLayoutGroup::setFrame( bool yn )
{
    if ( !yn )
	setFrameStyle( QFrame::NoFrame );
    else
    {
	widget_->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
	widget_->setLineWidth( 1 );
	widget_->setMidLineWidth( 0 );
    }
}


void uiLayoutGroup::setFrameStyle( int fs )
{
    widget_->setFrameStyle( fs );	
}


void uiLayoutGroup::setHAlignObj( const uiObject* obj )
{
    pErrMsg("Not supported");
}


void uiLayoutGroup::setHCenterObj( const uiObject* obj )
{
    pErrMsg("Not supported");
}


void uiLayoutGroup::setNoBackGround()
{
    widget_->setAttribute( Qt::WA_NoSystemBackground );
}


void uiLayoutGroup::setShrinkAllowed( bool yn )
{
    //widget_->setShrinkAllowed(yn);
}


bool uiLayoutGroup::shrinkAllowed()
{
    return false; //return widget_->shrinkAllowed();
}



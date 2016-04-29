/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiobjbody.h"
#include "uicursor.h"
#include "uimainwin.h"
#include "uimain.h"
#include "uigroup.h"
#include "uitreeview.h"

#include "color.h"
#include "settingsaccess.h"
#include "timer.h"
#include "perthreadrepos.h"

#include <QEvent>

mUseQtnamespace


mDefineEnumUtils(uiRect,Side,"Side") { "Left", "Right", "Top", "Bottom", 0 };

#define mBody_( imp_ )	dynamic_cast<uiObjectBody*>( imp_ )
#define mBody()		mBody_( body() )
#define mConstBody()	mBody_(const_cast<uiObject*>(this)->body())

uiParent::uiParent( const char* nm )
{}


void uiParent::addChild( uiObject& child )
{
    uiLayoutMgr* layoutmgr = getLayoutMgr();

    if ( !layoutmgr )
	return;

    layoutmgr->addObject( &child );

    //TODO: Add myself as parent? Or perhaps check?
}


void uiParent::attachChildren ( const uiObject* child1, constraintType tp,
				const uiObject* child2 )
{
    uiLayoutMgr* layoutmgr = getLayoutMgr();

    if ( !layoutmgr )
	return;

    layoutmgr->attach( child1, tp, child2 );
}


const ObjectSet<uiBaseObject>* uiParent::childList() const
{
    uiParentBody* uipb =
	    dynamic_cast<uiParentBody*>( const_cast<uiParent*>(this)->body() );
    return uipb ? uipb->childList(): 0;
}


Color uiObject::roBackgroundColor() const
{
    return backgroundColor().lighter( 2.5f );
}


Color uiParent::backgroundColor() const
{
    return mainObject() ? mainObject()->backgroundColor()
			: uiMain::theMain().windowColor();
}


void uiParent::translateText()
{
    uiBaseObject::translateText();

    if ( !childList() )
	return;

    for ( int idx=0; idx<childList()->size(); idx++ )
    {
	uiBaseObject* child = const_cast<uiBaseObject*>((*childList())[idx]);
	child->translateText();
    }
}


uiParentBody* uiParent::pbody()
{
    return dynamic_cast<uiParentBody*>( body() );
}


void uiParentBody::finaliseChildren()
{
    if ( !finalised_ )
    {
	finalised_= true;
	for ( int idx=0; idx<children_.size(); idx++ )
	    children_[idx]->finalise();
    }
}


void uiParentBody::clearChildren()
{
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->clear();
}

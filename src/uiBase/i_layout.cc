/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.cc,v 1.18 2001-09-20 13:26:17 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "errh.h"

#include "i_layout.h"
#include "i_layoutitem.h"
#include "uiobjbody.h"

#include <qlist.h>
#include <qmenubar.h>

#include <stdio.h>
#include <iostream>
#include <limits.h>


#define MAX_ITER	10000
#define NOT_OK_TRIGGER	 4832


int i_LayoutMngr::mintxtwidgethgt = -1;


//------------------------------------------------------------------------------

i_LayoutItem::i_LayoutItem( i_LayoutMngr& m, QLayoutItem& itm ) 
:   mngr_( m ), mQLayoutItem_( itm ), 
    preferred_pos_inited( false ), minimum_pos_inited( false )
{}


i_LayoutItem::~i_LayoutItem()
{
    delete &mQLayoutItem_;

    constraintIterator it = iterator();
    uiConstraint* c;
    while ( (c = it.current()) )
    {
	++it;
	delete c;
    }
}

void i_LayoutItem::invalidate() 
{ 
    mQLayoutItem_.invalidate();
    preferred_pos_inited = false;
    minimum_pos_inited = false;
}


constraintIterator i_LayoutItem::iterator()
{
    return constraintIterator(constrList);
}


uiSize i_LayoutItem::actualSize( bool include_border ) const
    { return pos(setGeom).size(); }


int i_LayoutItem::stretch( bool hor ) const
{ 
    const uiObjectBody* blo = bodyLayouted();
    return blo ? blo->stretch( hor ) : 0; 
}


void i_LayoutItem::commitGeometrySet()
{
    uiRect mPos = pos( setGeom );

    if( objLayouted() ) objLayouted()->triggerSetGeometry( this, mPos );

//#define DEBUG_LAYOUT 
#ifdef DEBUG_LAYOUT

    BufferString msg;
    if( objLayouted() )
    {   
	msg = "setting geometry on: ";
	msg +=  objLayouted()->name();
	msg += "; top: ";
        msg += mPos.top();
        msg += " left: ";
        msg += mPos.left();
	msg += " with: ";
        msg += mPos.width();
	msg += " height: ";
        msg += mPos.height();

    }
    else msg = "not a uiLayout item..";
    pErrMsg( msg ); 

    QRect geom ( mPos.left(), mPos.top(), mPos.width(), mPos.height());

    mQLayoutItem_.setGeometry ( geom ); 

#else

    mQLayoutItem_.setGeometry ( QRect ( mPos.left(), mPos.top(), 
                                       mPos.width(), mPos.height() )); 
#endif
}

void i_LayoutItem::initLayout( layoutMode m, int mngrTop, int mngrLeft )
{
    uiRect& mPos = pos( m );
    int preferred_width;
    int preferred_height;

    if( objLayouted() )
    {
	preferred_width  = objLayouted()->preferredWidth();
	preferred_height = objLayouted()->preferredHeight();
    }
    else
    {
	QSize sh(mQLayoutItem_.sizeHint());
	preferred_width  = sh.width();
	preferred_height = sh.height();
    }

#ifdef DEBUG_LAYOUT

    BufferString msg;
    msg = "initLayout on ";
    msg+= objLayouted() ? const_cast<char*>((const char*)objLayouted()->name()) : "UNKNOWN";      
    msg+= "  mngrTop: ";
    msg+= mngrTop;
    msg+= "  mngrLeft: ";
    msg+= mngrLeft;

    pErrMsg( msg );

#endif
    switch ( m )
    {
	case minimum:
            if( !minimum_pos_inited)
	    {
		mPos.zero();
           
                if( stretch(true) )
		    mPos.setWidth( minimumSize().width() );
                else
		    mPos.setWidth( preferred_width );

                if( stretch(false) )
		    mPos.setHeight( minimumSize().height() );
                else
		    mPos.setHeight( preferred_height );

		minimum_pos_inited = true;
	    }
	    break;

	case setGeom:
	    if( !preferred_pos_inited )
	    {
		uiRect& pPos = pos(preferred);
		pPos.setLeft( mngrLeft );
		pPos.setTop( mngrTop );

		pPos.setWidth ( preferred_width  );
		pPos.setHeight( preferred_height );
		preferred_pos_inited = true;
	    }

	    mPos.setLeft( mngrLeft );
	    mPos.setTop( mngrTop );

	    mPos.setWidth ( preferred_width  );
	    mPos.setHeight( preferred_height );

	    break;

	case preferred:
	    mPos.setLeft( mngrLeft );
	    mPos.setTop( mngrTop );

	    mPos.setWidth ( preferred_width  );
	    mPos.setHeight( preferred_height );
	    preferred_pos_inited = true;
	    break;
    } 
}


int i_LayoutItem::isPosOk( uiConstraint* c, int i )
{
    if( i < NOT_OK_TRIGGER ) return i;

    if( c->enabled() ) 
    {
	BufferString msg;

	msg = "\n  Layout loop on: \"";
	msg+= objLayouted() ? (const char*)objLayouted()->name() : "UNKNOWN";
	msg+= "\"";

	switch ( c->type )
	{
	    case leftOf: 		msg+= " leftOf "; break;
	    case rightOf:		msg+= " rightOf "; break;
	    case leftTo:		msg+= " leftTo "; break;
	    case rightTo:		msg+= " rightTo "; break;
	    case leftAlignedBelow:	msg+= " leftAlignedBelow "; break;
	    case leftAlignedAbove:	msg+= " leftAlignedAbove "; break;
	    case rightAlignedBelow:	msg+= " rightAlignedBelow "; break;
	    case rightAlignedAbove:	msg+= " rightAlignedAbove "; break;
	    case alignedBelow:		msg+= " alignedBelow "; break;
	    case alignedAbove:		msg+= " alignedAbove "; break;
	    case centeredBelow:		msg+= " centeredBelow "; break;
	    case centeredAbove:		msg+= " centeredAbove "; break;
	    case ensureLeftOf:		msg+= " ensureLeftOf "; break;
	    case ensureRightOf:		msg+= " ensureRightOf "; break;
	    case ensureBelow:		msg+= " ensureBelow "; break;
	    case leftBorder:		msg+= " leftBorder "; break;
	    case rightBorder:		msg+= " rightBorder "; break;
	    case topBorder:		msg+= " topBorder "; break;
	    case bottomBorder:		msg+= " bottomBorder "; break;
	    case heightSameAs: 		msg+= " heightSameAs "; break;
	    case widthSameAs:		msg+= " widthSameAs "; break;
	    case stretchedBelow:	msg+= " stretchedBelow "; break;
	    case stretchedAbove:	msg+= " stretchedAbove "; break;
	    case stretchedLeftTo:	msg+= " stretchedLeftTo "; break;
	    case stretchedRightTo:	msg+= " stretchedRightTo "; break;
	    default:		 	msg+= " .. "; break;
	}

	msg+= "\"";
	msg+= c->other->objLayouted() ? 
		    (const char*)c->other->objLayouted()->name() : "UNKNOWN";
	msg+= "\"";
	pErrMsg( msg );

	c->disable();
    }
    return i;
}


#ifdef __debug__
#define mCP(val)	isPosOk(constr,(val))
#define mUpdated()	{ isPosOk(constr,MAX_ITER-iteridx); *chupd=true; }
#else
#define mCP(val)	(val)
#define mUpdated()	{ *chupd=true; }
#endif

void i_LayoutItem::layout(layoutMode m, const int iteridx, bool* chupd )
{
//    if ( !constrList ) return;

#define mHorSpacing (constr->margin >= 0 ? constr->margin : mngr_.horSpacing())
#define mVerSpacing (constr->margin >= 0 ? constr->margin : mngr_.verSpacing())


    uiRect& mPos = pos(m);

    constraintIterator it = iterator();

    uiConstraint* constr;
    while ( (constr = it.current()) )
    {
	++it;

	uiRect otherPos = constr->other ? constr->other->pos(m) : uiRect();

	switch ( constr->type )
	{
	    case rightOf:
	    case rightTo:
		if( mPos.leftToAtLeast( mCP(otherPos.right() + mHorSpacing)))  
		    mUpdated(); 
		if ( mPos.topToAtLeast( mCP(otherPos.top()) ) ) 
		     mUpdated();
		break;
	    case leftOf:  
		if( mPos.rightToAtLeast( mCP(otherPos.left() - mHorSpacing)) )  
		    mUpdated(); 
		if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		     mUpdated();
		break;
	    case leftTo:  
		if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		     mUpdated();
		break;
		      
	    case leftAlignedBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated(); 
		if( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		    mUpdated();
		break;

	    case leftAlignedAbove: 
		if( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		    mUpdated();
		break;

	    case rightAlignedBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated();
		if( mPos.rightToAtLeast( mCP(otherPos.right())) )
		    mUpdated();
		break;

	    case rightAlignedAbove: 
		if( mPos.rightToAtLeast( mCP(otherPos.right()) ) )
		    mUpdated();
		break;

	    case alignedBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated();
		if( mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horAlign(m) 
					- horAlign(m)) 
				      )
		  ) 
		    mUpdated();
		break;

	    case alignedAbove: 
		if( mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horAlign(m) 
					- horAlign(m)) 
				      )
		  ) 
		    mUpdated();
		break;

	    case centeredBelow:
	    {
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		    mUpdated(); 

		if( horCentre(m) > 0 && constr->other->horCentre(m) > 0 &&
		    mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horCentre(m) 
					- horCentre(m)) 
				      )
		  ) 
		    mUpdated();
		break;
	    }
	    case centeredAbove: 
            {
		if( horCentre(m) > 0 && constr->other->horCentre(m) > 0 &&
		    mPos.leftToAtLeast( mCP(mPos.left() 
					+ constr->other->horCentre(m) 
					- horCentre(m)) 
				      )
		  ) 
		    mUpdated();
		break;
            } 
	    case ensureRightOf:
		if( mPos.leftToAtLeast( mCP(otherPos.right() + mHorSpacing )))  
		    mUpdated(); 
		break;

	    case ensureBelow:
		if( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing ))) 
		    mUpdated(); 
		break;
	    case leftBorder:
		{
		    int nwLeft = mngr().pos(m).left() + mHorSpacing;
		    if( mPos.left() != nwLeft )
		    {
			mPos.leftTo( mCP(nwLeft));
			mUpdated();
		    }
		}
		break;
	    case rightBorder:
		{
		    int nwRight = mngr().pos(m).right() - mHorSpacing;
		    if( mPos.rightToAtLeast( mCP(nwRight) ) ) mUpdated();
		}
		break;
	    case topBorder:
		{
		    int nwTop = mngr().pos(m).top() + mVerSpacing;
		    if( mPos.top() != nwTop )
		    {
			mPos.topTo( mCP(nwTop ));
			mUpdated();
		    }
		}
		break;
	    case bottomBorder:
		{
		    int nwBottom = mngr().pos(m).bottom() - mVerSpacing;
		    if( mPos.bottomToAtLeast( mCP(nwBottom )))
			mUpdated();
		}
		break;
	    case heightSameAs:
		if( mPos.height() < ( otherPos.height() ) )
		{
		    mPos.setHeight( otherPos.height() );
		    mUpdated();
		}
		break;
	    case widthSameAs:
		if( mPos.width() < ( otherPos.width() ) )
		{
		    mPos.setWidth( otherPos.width() );
		    mUpdated();
		}
		break;
	    case stretchedBelow:
		{
		    int nwWidth = mngr().pos(m).width();
		    if( mPos.width() < nwWidth )
		    {
			mPos.setWidth( nwWidth );
			mUpdated();
		    }
		    if( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
			mUpdated(); 
		}
		break;
	    case stretchedAbove:
		{
		    int nwWidth = mngr().pos(m).width();
		    if( mPos.width() < nwWidth )
		    {
			mPos.setWidth( nwWidth );
			mUpdated();
		    }
		}
		break;
	    case stretchedLeftTo:
		{
		    int nwHeight = mngr().pos(m).height();
		    if( mPos.height() < nwHeight )
		    {
			mPos.setHeight( nwHeight );
			mUpdated();
		    }
		}
		break;
	    case stretchedRightTo:
		{
		    int nwHeight = mngr().pos(m).height();
		    if( mPos.height() < nwHeight )
		    {
			mPos.setHeight( nwHeight );
			mUpdated();
		    }
		    if( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
			mUpdated(); 
		}
		break;
	    case ensureLeftOf:
		break;
	    default:
		pErrMsg("Unknown constraint type");
		break;

	}


    }
}


void i_LayoutItem::attach ( constraintType type, i_LayoutItem *other, 
			    int margn )
{
    if( type != ensureLeftOf)
	constrList.append( new uiConstraint( type, other, margn ) );

    switch ( type )
    {
	case leftOf :
	    other-> constrList.append(new uiConstraint( rightOf, this, margn));
	    break;
	case rightOf:
	    other-> constrList.append(new uiConstraint( leftOf, this, margn ));
	    break;
	case leftTo :
	    other-> constrList.append(new uiConstraint( rightTo, this, margn));
	    break;
	case rightTo:
	    other-> constrList.append(new uiConstraint( leftTo, this, margn ));
	    break;
	case leftAlignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( leftAlignedAbove, this, margn));
	    break;
	case leftAlignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( leftAlignedBelow, this, margn ));
	    break;
	case rightAlignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( rightAlignedAbove, this, margn));
	    break;
	case rightAlignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( rightAlignedBelow, this, margn));
	    break;
	case alignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( alignedAbove, this, margn ) );
	    break;
	case alignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( alignedBelow, this, margn ) );
	    break;
	case centeredBelow:
	    other-> constrList.append( 
			    new uiConstraint( centeredAbove, this, margn ) );
	    break;
	case centeredAbove:
	    other-> constrList.append( 
			    new uiConstraint( centeredBelow, this, margn ) );
	    break;

	case heightSameAs:
	case widthSameAs:
	    other-> constrList.append( new uiConstraint( type, this, margn ) );
	    break;

	case ensureLeftOf:
	    other-> constrList.append( 
			    new uiConstraint( ensureRightOf, this, margn ) );
	    break;
	case stretchedBelow:
	    break;
	case stretchedAbove:
	    other-> constrList.append( 
			    new uiConstraint( ensureBelow, this, margn ) );
	    break;
	case stretchedLeftTo:
	    other-> constrList.append( 
			    new uiConstraint( stretchedRightTo, this, margn ) );
	    break;
	case stretchedRightTo:
	    other-> constrList.append( 
			    new uiConstraint( stretchedLeftTo, this, margn ) );
	    break;

	case leftBorder:
	case rightBorder:
	case topBorder:
	case bottomBorder:
	case ensureRightOf:
	case ensureBelow:
	    break;
	default:
	    pErrMsg("Unknown constraint type");
	    break;
    }
}


//------------------------------------------------------------------

class i_LayoutIterator :public QGLayoutIterator
{
public:
    			i_LayoutIterator( QList<i_LayoutItem> *l ) 
			: idx(0), list(l)  {}

    uint		count() 		const;
    QLayoutItem*	current();
    QLayoutItem*	next();
    QLayoutItem*	takeCurrent();
    i_LayoutItem*	takeCurrent_();

private:
    int idx;
    QList<i_LayoutItem> *list;

};

uint i_LayoutIterator::count() const
{
    return list->count();
}

QLayoutItem* i_LayoutIterator::current()
{
    return idx < int(count()) ? &(list->at( idx )->mQLayoutItem()) : 0;
}

QLayoutItem* i_LayoutIterator::next()
{
    idx++; return current();
}

QLayoutItem* i_LayoutIterator::takeCurrent()
{
    return idx < int(count()) ? &(list->take( idx )->mQLayoutItem()) : 0;
}

i_LayoutItem* i_LayoutIterator::takeCurrent_()
{
    return idx < int(count()) ? (list->take( idx )) : 0;
}

//-----------------------------------------------------------------

i_LayoutMngr::i_LayoutMngr( QWidget* parnt, int border, int space,
			    const char *name )
			    : QLayout( parnt, border, space, name)
			    , UserIDObject( name )
			    , prevGeometry()
			    , minimumDone( false )
			    , preferredDone( false )
{}


i_LayoutMngr::~i_LayoutMngr()
{
    i_LayoutIterator it( &childrenList );
    i_LayoutItem *l;
    while ( (l=it.takeCurrent_()) )
	delete l;
}


void i_LayoutMngr::addItem( i_LayoutItem* itm )
{
    if( !itm ) return;
    childrenList.append( itm );
}


/*! \brief Adds a QlayoutItem to the manager's children

    Should normally not been called, since all ui items are added to the
    parent's manager using _LayoutMngr::addItem( i_LayoutItem* itm )

*/
void i_LayoutMngr::addItem( QLayoutItem *qItem )
{
    if( !qItem ) return;
    childrenList.append( new i_LayoutItem( *this, *qItem) );
}


QSize i_LayoutMngr::minimumSize() const
{
    if ( !minimumDone ) 
    { 
	doLayout( minimum, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->minimumDone=true; 
    }
    uiRect mPos = pos(minimum);
    return QSize( mPos.width(), mPos.height() );
}

QSize i_LayoutMngr::sizeHint() const
{
    if ( !preferredDone )
    { 
	doLayout( preferred, QRect() ); 
	const_cast<i_LayoutMngr*>(this)->preferredDone=true; 
    }
    uiRect mPos = pos(preferred);
    return QSize( mPos.width(), mPos.height() );
}

QSizePolicy::ExpandData i_LayoutMngr::expanding() const
{
    return QSizePolicy::BothDirections;
}

QLayoutIterator i_LayoutMngr::iterator()
{
    return QLayoutIterator( new i_LayoutIterator( &childrenList ) ) ;
}

//! \internal class used when resizing a window
class resizeItem
{
public:
			resizeItem( i_LayoutItem* it, int hStre, int vStre ) 
                        : item( it ), hStr( hStre ), vStr( vStre )
                        , hDelta( 0 ), vDelta( 0 ) 
			, nhiter( hStre? 2 : 0 ), nviter( vStre ? 2 : 0 ) {}

    i_LayoutItem* 	item;
    const int 		hStr;
    const int 		vStr;
    int			nhiter;
    int			nviter;
    int			hDelta;
    int			vDelta;

};


void i_LayoutMngr::childrenClear( uiObject* cb )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	uiObject* cl = curChld->objLayouted();
	if( cl && cl != cb ) cl->clear();
    }
}

int i_LayoutMngr::childStretch( bool hor ) const
{
    QListIterator<i_LayoutItem> childIter( childrenList );

    int sum=0;
    childIter.toFirst();
    while ( i_LayoutItem* curChld = childIter.current() )
    {
        ++childIter;
	uiObjectBody* ccbl = curChld->bodyLayouted();
	if( ccbl ) sum += ccbl->stretch( hor );
    }

    return sum;
}


void i_LayoutMngr::forceChildrenRedraw( uiObjectBody* cb, bool deep )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	uiObjectBody* cl = curChld->bodyLayouted();
	if( cl && cl != cb ) cl->reDraw( deep );
    }

}

void i_LayoutMngr::fillResizeList( ObjectSet<resizeItem>& resizeList, 
				   int& maxh, int& maxv,
				   int& nrh, int& nrv )
{
    QListIterator<i_LayoutItem> childIter( childrenList );
    childIter.toFirst();

    maxh=0;
    maxv=0;
    nrh=0;
    nrv=0;

    while ( i_LayoutItem* curChld = childIter.current() )
    {
        ++childIter;
	int hs = curChld->stretch(true);
	int vs = curChld->stretch(false);

	if ( hs || vs )
        {
	    if( hs )
	    {
		nrh++;
		if( hs > maxh ) maxh = hs;
	    }
	    if( vs )
	    {
		nrv++;
		if( vs > maxv ) maxv = vs;
	    }

	    resizeList += new resizeItem( curChld, hs, vs );
        }
   } 
}


void i_LayoutMngr::moveChildrenTo(int rTop, int rLeft, layoutMode m )
{
    QListIterator<i_LayoutItem> childIter( childrenList );
    childIter.toFirst();
    while ( i_LayoutItem* curChld = childIter.current() )
    {
	++childIter;
	uiRect& chldGeomtry  = curChld->pos(m);
	chldGeomtry.topTo ( rTop );
	chldGeomtry.leftTo ( rLeft );
    }
}

bool i_LayoutMngr::tryToGrowItem( resizeItem& cur, const QRect& targetRect )
{
    layoutChildren( setGeom );
    uiRect childrenBBox = childrenRect(setGeom);  

    if(   ( childrenBBox.width() > targetRect.width() )
	||( childrenBBox.height() > targetRect.height() ))
	{ pErrMsg("huh"); return false; }

    bool done_something = false;

    uiRect& myGeomtry  = cur.item->pos( setGeom );
    const uiRect& refGeom = cur.item->pos( minimum );

    bool hdone = false;
    bool vdone = false;


    if( cur.nhiter ) 
    {
	hdone = true;
	myGeomtry.setWidth ( refGeom.width() + ++cur.hDelta );
    }

    if(  cur.nviter )
    {
	vdone = true;
	myGeomtry.setHeight( refGeom.height() + ++cur.vDelta );
    }
   
    layoutChildren( setGeom );
    childrenBBox = childrenRect(setGeom);  

    bool do_layout = false;

    if( hdone && ( childrenBBox.width() > targetRect.width()) )  
    { 
	cur.nhiter--;
	cur.hDelta--;

	myGeomtry.setWidth( refGeom.width() + cur.hDelta );
	do_layout = true;
    }
    else { done_something = true; }

    if( vdone && (childrenBBox.height() > targetRect.height() ))
    {   
	cur.nviter--;
	cur.vDelta--;

	myGeomtry.setHeight( refGeom.height() + cur.vDelta );
	do_layout = true;
    }
    else { done_something = true; }

    if( do_layout ) 
    {   // move all items to top-left corner first 
	moveChildrenTo(targetRect.top(),targetRect.left(),setGeom);

	layoutChildren( setGeom );
	childrenBBox = childrenRect(setGeom);  
    } 

    if( ( childrenBBox.width() > targetRect.width() ) )
	{ pErrMsg("hShit!"); return false; }
    if	( childrenBBox.height() > targetRect.height() )
	{ pErrMsg("vShit!"); return false; }

    return done_something;
}


void i_LayoutMngr::resizeTo( const QRect& targetRect )
{

    doLayout( setGeom, targetRect );//init to prefer'd size and initial layout
    uiRect childrenBBox = childrenRect(setGeom);  

    int hSpace = targetRect.width() - childrenBBox.width();
    int vSpace = targetRect.height() - childrenBBox.height();


    if( (!hSpace && !vSpace) || hSpace<0 || vSpace<0 ) return;

    ObjectSet<resizeItem> resizeList;
    int maxHstr, maxVstr;
    int nrHstr, nrVstr;
    fillResizeList( resizeList, maxHstr, maxVstr, nrHstr, nrVstr );

    int iter = MAX_ITER;

#if 0
    while( (hSpace || vSpace) && (hSpace >= 0) && (vSpace >= 0) && iter )
    {   

	int hDelta = maxHstr ? hSpace / maxHstr : 0;
	int vDelta = maxVstr ? vSpace / maxVstr : 0;

	if( !hDelta && !vDelta )
	    break;

	for( int idx=0; idx<resizeList.size(); idx++ )
	{
	    resizeItem* cur = resizeList[idx];

	    uiRect& myGeomtry  = cur->item->pos( setGeom );
	    const uiRect& refGeom = cur->item->pos( preferred );


	    if( hDelta && cur->hStr ) 
	    {
		cur->hDelta += cur->hStr*hDelta;
		myGeomtry.setWidth ( refGeom.width() + cur->hDelta );
	    }

	    if( vDelta && cur->vStr ) 
	    {
		cur->vDelta += cur->vStr*vDelta;
		myGeomtry.setHeight( refGeom.height() + cur->vDelta );
	    }
	}

	layoutChildren( setGeom );
	childrenBBox = childrenRect(setGeom);  

	hSpace = targetRect.width() - childrenBBox.width();
	vSpace = targetRect.height() - childrenBBox.height();

	if( (hSpace < 0) || (vSpace < 0) ) 
	    { pErrMsg("Space left < 0");  }
    }
#endif

    for( bool go_on = true; go_on && iter; iter--)
    {   
	go_on = false;
	for( int idx=0; idx<resizeList.size(); idx++ )
	{
	    resizeItem* cur = resizeList[idx];
	    if( cur && (cur->nhiter || cur->nviter)) 
	    { 
		if( tryToGrowItem( *cur, targetRect ) ) 
		    go_on = true; 
	    }
	}
    }

    deepErase( resizeList );
    if ( !iter ) pErrMsg("Stopped resize. Too many iterations ");
}

void i_LayoutMngr::setGeometry( const QRect &extRect )
{
    if( extRect == prevGeometry ) return;
    prevGeometry = extRect;

    QSize minSz = minimumSize();
    QRect targetRect = extRect;

    if( targetRect.width() < minSz.width() )   
    { targetRect.setWidth( minSz.width() ); }
    if( targetRect.height() < minSz.height() ) 
    { targetRect.setHeight( minSz.height() ); }

    QLayout::setGeometry( extRect );

    resizeTo( targetRect );
    childrenCommitGeometrySet();
}

void i_LayoutMngr::childrenCommitGeometrySet()
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    while ( (curChld = childIter.current()) )
    {
        ++childIter;
	curChld->commitGeometrySet();
    }
}


void i_LayoutMngr::doLayout( layoutMode m, const QRect &externalRect )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    bool geomSetExt = ( externalRect.width() && externalRect.height() );
    if( geomSetExt )
    {
	pos(m) = uiRect(externalRect.left(), externalRect.top(), 
	    externalRect.right(), externalRect.bottom());
    }

    childIter.toFirst(); 
    while ( (curChld = childIter.current()) ) 
    { 
	++childIter;
	uiObjectBody* cl = curChld->bodyLayouted();
	if( cl && cl->isSingleLine() )
	{ 
	    int chldPref = cl->preferredHeight();
	    if( chldPref > mintxtwidgethgt ) 
		mintxtwidgethgt = chldPref;
	}
    }

    int mngrTop  = externalRect.top();
    int mngrLeft = externalRect.left();

    childIter.toFirst(); 
    while ( (curChld = childIter.current()) ) 
    { 
	++childIter; 
	curChld->initLayout( m, mngrTop, mngrLeft ); 
    }

    layoutChildren(m);

    if( !geomSetExt )
	pos(m) = childrenRect(m);
}

void i_LayoutMngr::layoutChildren( layoutMode m )
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    bool child_updated = true;
    int iter = MAX_ITER;
 
    while ( child_updated && iter ) 
    {
        if( iter ) iter--;
        child_updated = false;
        childIter.toFirst();
	while ( (curChld = childIter.current()) )
	{ 
	    ++childIter;
	    curChld->layout(m, iter ,&child_updated); 
	}
	//pos_[ curMode() ] = childrenRect();
    }
    if ( !iter ) 
      { pErrMsg("Stopped layout. Too many iterations "); }
}

uiRect i_LayoutMngr::childrenRect( layoutMode m )
//!< returns rectangle wrapping around all children.
{
    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );

    childIter.toFirst();
    curChld = childIter.current();
    uiRect chldRect;
    if( curChld ) 
    {
	const uiRect* childPos = &curChld->pos(m);

	chldRect.setTopLeft( childPos->topLeft() );
	chldRect.setBottomRight( childPos->bottomRight() );

	while ( (curChld = childIter.current()) )
	{
	    ++childIter;
	    childPos = &curChld->pos(m);


	    if ( childPos->top() < chldRect.top() ) 
					chldRect.setTop( childPos->top() );
	    if ( childPos->left() < chldRect.left() ) 
					chldRect.setLeft( childPos->left() );
	    if ( childPos->right() > chldRect.right() ) 
					chldRect.setRight( childPos->right() );
	    if ( childPos->bottom() > chldRect.bottom() ) 
					chldRect.setBottom( childPos->bottom());
	}
    }
    return chldRect;
}


void i_LayoutMngr::invalidate() 
{ 

    prevGeometry = QRect();
    minimumDone = false;
    preferredDone = false;

    i_LayoutItem*       	curChld=0;
    QListIterator<i_LayoutItem> childIter( childrenList );
    childIter.toFirst(); 

    while ( (curChld = childIter.current()) ) 
    { 
	++childIter; 
	curChld->invalidate(); 
    }
}


bool i_LayoutMngr::attach ( constraintType type, QWidget& current, 
			    QWidget* other, int margin) 
//!< \return true if successful
{
    QListIterator<i_LayoutItem> childIter( childrenList );
   
    childIter.toFirst();
    i_LayoutItem *loop, *cur=0, *oth=0;

    if (&current == other)
	{ pErrMsg("Cannot attach an object to itself"); return false; }
    
    while ( ( loop = childIter.current() ) && !(cur && oth) ) 
    {
        ++childIter;
	if( loop->qwidget() == &current) cur = loop;
        if( loop->qwidget() == other)   oth = loop;
    }

    if (cur && ((!oth && !other) || (other && oth && (oth->qwidget()==other)) ))
    {
	cur->attach( type, oth, margin );
	return true;
    }

    const char* curnm =  current.name();
    const char* othnm =  other ? other->name() : "";
    
    BufferString msg( "i_LayoutMngr :: Cannot attach " );
    msg += curnm;
    msg += " and ";
    msg += othnm;
    pErrMsg( msg );

    return false;
}
/*
void i_LayoutMngr::setLayoutPosItm( i_LayoutItem* itm )
{
    if ( !itm ) 
	{ pErrMsg("i_LayoutItem* == 0"); return; }
    if ( layoutpos )
	{ pErrMsg("Layoutpos aready set"); return; }
    layoutpos = itm->layoutpos;
}
*/

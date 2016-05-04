/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiparent.h"
#include "uicursor.h"
#include "uifont.h"
#include "uimainwin.h"
#include "uimain.h"
#include "uigroup.h"
#include "uilayout.h"
#include "uitreeview.h"
#include "pixmap.h"

#include "color.h"
#include "settingsaccess.h"
#include "timer.h"
#include "perthreadrepos.h"

#include <QEvent>
#include <QWidget>

mUseQtnamespace


mDefineEnumUtils(uiRect,Side,"Side") { "Left", "Right", "Top", "Bottom", 0 };


class uiObjEventFilter : public QObject
{
public:
			uiObjEventFilter( uiObject& uiobj )
			    : uiobject_( uiobj )
			{}
protected:
    bool		eventFilter(QObject*,QEvent*);
    uiObject&		uiobject_;
};


bool uiObjEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    if ( ev && ev->type() == mUsrEvLongTabletPress )
    {
	uiobject_.handleLongTabletPress();
	return true;
    }

    return false;
}


static ObjectSet<uiObject> uiobjectlist_;


static BufferString getCleanName( const char* nm )
{
    QString qstr( nm );
    qstr.remove( QChar('&') );
    return BufferString( qstr );
}

static int iconsz_ = -1;
static int fldsz_ = -1;

uiObject::uiObject( uiParent* p, const char* nm )
    : uiBaseObject( getCleanName(nm) )
    , setGeometry(this)
    , closed(this)
    , parent_( p )
    , singlewidget_( 0 )
{
    if ( p ) p->addChild( *this );
    uiobjectlist_ += this;
    updateToolTip();

    uiobjeventfilter_ = new uiObjEventFilter( *this );
}


uiObject::~uiObject()
{
    if ( singlewidget_ )
	singlewidget_->removeEventFilter( uiobjeventfilter_ );

    delete uiobjeventfilter_;

    closed.trigger();
    uiobjectlist_ -= this;
}


void uiObject::setSingleWidget( QWidget* w )
{
    if ( singlewidget_ )
	singlewidget_->removeEventFilter( uiobjeventfilter_ );

    singlewidget_ = w;

    if ( singlewidget_ )
	singlewidget_->installEventFilter( uiobjeventfilter_ );
}


void uiObject::setHSzPol( SzPolicy p )
    {  }

void uiObject::setVSzPol( SzPolicy p )
    {  }

uiObject::SzPolicy uiObject::szPol(bool hor) const
{ return Undef; }


void uiObject::setName( const char* nm )
{
    uiBaseObject::setName( getCleanName(nm) );
    updateToolTip();
}


const uiString& uiObject::toolTip() const
{
    return tooltip_;
}


void uiObject::setToolTip( const uiString& txt )
{
    tooltip_ = txt;
    updateToolTip();
}


void uiObject::updateToolTip(CallBacker*)
{
    mEnsureExecutedInMainThread( uiObject::updateToolTip );
    if ( !getWidget(0) ) return;

    if ( uiMain::isNameToolTipUsed() && !name().isEmpty() )
    {
	BufferString namestr = name().buf();
	uiMain::formatNameToolTipString( namestr );
	getWidget(0)->setToolTip( namestr.buf() );
    }
    else
	getWidget(0)->setToolTip( tooltip_.getQString() );
}


void uiObject::translateText()
{
    uiBaseObject::translateText();
    updateToolTip();
}


void uiObject::display( bool yn, bool shrink, bool maximize )
{
    finalise();
    if ( shrink || maximize )
    {
        pErrMsg("Shrink and maximize not implemented");
    }
    
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget(idx)->setShown( yn );
    }
}

void uiObject::setFocus()
{
    if ( singlewidget_ ) singlewidget_->setFocus();
    else pErrMsg("multiwidget not supported");
}

bool uiObject::hasFocus() const
{ return singlewidget_ ? singlewidget_->hasFocus() : false; }

void uiObject::disabFocus()
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget( idx )->setFocusPolicy( Qt::NoFocus );
    }
}


void uiObject::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget( idx )->setCursor( qcursor );
    }
}


bool uiObject::isCursorInside() const
{
    const uiPoint cursorpos = uiCursorManager::cursorPos();
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        const QWidget* widget = getConstWidget( idx );
        const QPoint objpos = widget->mapToGlobal( QPoint(0,0) );
        if ( cursorpos.x>=objpos.x() &&
             cursorpos.x<objpos.x()+widget->width() &&
             cursorpos.y>=objpos.y() &&
             cursorpos.y<objpos.y()+widget->height() )
        {
            return true;
        }
    }
    
    return false;
}


void uiObject::setStyleSheet( const char* qss )
{
    if ( singlewidget_ )
        singlewidget_->setStyleSheet( qss );
}


Color uiObject::backgroundColor() const
{
    return singlewidget_
    	? Color(singlewidget_->palette().brush(
                singlewidget_->backgroundRole() ).color().rgb() )
    : Color(0,0,0,0);
}


void uiObject::setBackgroundColor(const Color& col)
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        QPalette qpal( getWidget(idx)->palette() );
        qpal.setColor( QPalette::Base,
                      QColor(col.r(),col.g(),col.b(),255-col.t()) );
        getWidget(idx)->setPalette( qpal );
    }
}


void uiObject::setBackgroundPixmap( const uiPixmap& pm )
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        QPalette qpal( getWidget(idx)->palette() );

        qpal.setBrush( getWidget(idx)->backgroundRole(), QBrush(*pm.qpixmap()) );
        getWidget(idx)->setPalette( qpal );
    }
}


void uiObject::setTextColor(const Color& col)
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        QPalette qpal( getWidget( idx )->palette() );
        qpal.setColor( QPalette::WindowText,
                      QColor(col.r(),col.g(),col.b(),255-col.t()) );

        getWidget( idx )->setPalette( qpal );
    }
}


void uiObject::setSensitive(bool yn)
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget( idx )->setEnabled( yn );
    }
}


bool uiObject::sensitive() const
{ return getNrWidgets() && getConstWidget(0)->isEnabled(); }


bool uiObject::visible() const
    { return getNrWidgets() && getConstWidget(0)->isVisible(); }

bool uiObject::isDisplayed() const
{
    return getNrWidgets() && getConstWidget(0)->isVisible();
}

int uiObject::prefHNrPics() const
{ return mUdf(int); }


void uiObject::setPrefWidth( int w )
    { /* mBody()->setPrefWidth(w); */ }


void uiObject::setPrefWidthInChar( int w )
    { /* mBody()->setPrefWidthInChar( (float)w ); */ }

void uiObject::setPrefWidthInChar( float w )
     { /* mBody()->setPrefWidthInChar(w); */ }

void uiObject::setMinimumWidth( int w )
    { /* mBody()->setMinimumWidth(w);  */}

void uiObject::setMinimumHeight( int h )
    { /* mBody()->setMinimumHeight(h);  */}

void uiObject::setMaximumWidth( int w )
    { /* mBody()->setMaximumWidth(w);  */}

void uiObject::setMaximumHeight( int h )
    { /* mBody()->setMaximumHeight(h);  */}

int uiObject::prefVNrPics() const
{ return mUdf(int); /* return mConstBody()->prefVNrPics();  */ }

void uiObject::setPrefHeight( int h )
    { /* mBody()->setPrefHeight(h); */ }

void uiObject::setPrefHeightInChar( int h )
    { /* mBody()->setPrefHeightInChar( (float)h ); */ }

void uiObject::setPrefHeightInChar( float h )
     {/* mBody()->setPrefHeightInChar(h); */}

void uiObject::setStretch( int hor, int ver )
     {/* mBody()->setStretch(hor,ver);  */}

void uiObject::attach ( constraintType tp, uiObject* other )
{
    if ( parent() && parent()->getLayoutMgr() )
    {
        parent()->getLayoutMgr()->attach( this, tp, other );
    }
}

/*!
    Moves the \a second widget around the ring of focus widgets so
    that keyboard focus moves from the \a first widget to the \a
    second widget when the Tab key is pressed.

    Note that since the tab order of the \a second widget is changed,
    you should order a chain like this:

    \code
	setTabOrder( a, b ); // a to b
	setTabOrder( b, c ); // a to b to c
	setTabOrder( c, d ); // a to b to c to d
    \endcode

    \e not like this:

    \code
	setTabOrder( c, d ); // c to d   WRONG
	setTabOrder( a, b ); // a to b AND c to d
	setTabOrder( b, c ); // a to b to c, but not c to d
    \endcode

    If \a first or \a second has a focus proxy, setTabOrder()
    correctly substitutes the proxy.
*/
void uiObject::setTabOrder( uiObject* first, uiObject* second )
{
    if ( first->getNrWidgets()!=1  || second->getNrWidgets()!=1 )
        return;
    
    QWidget::setTabOrder( first->getWidget(0), second->getWidget(0) );
}


void uiObject::setFont( const uiFont& f )
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget( idx )->setFont( f.qFont() );
    }
}

const uiFont* uiObject::font() const
{
    if ( !getNrWidgets() )
        return 0;
    
    QFont qf( getConstWidget(0)->font() );
    return &FontList().getFromQfnt(&qf);
}


uiSize uiObject::actualsize( bool include_border ) const
{ return uiSize( mUdf(int), mUdf(int)); }

void uiObject::setCaption( const uiString& c )
{
    if ( singlewidget_ )
        singlewidget_->setWindowTitle( c.getQString() );
}


void uiObject::reDraw( bool )
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget( idx )->update();
    }
}


uiMainWin* uiObject::mainwin()
{
    uiParent* par = parent();
    if ( !par )
    {
	mDynamicCastGet(uiMainWin*,mw,this)
	return mw;
    }

    return par->mainwin();
}


QWidget* uiObject::getWidget( int )
{ return singlewidget_; }


void uiObject::close()
{
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
        getWidget( idx )->close();
    }
}


int uiObject::width() const
{
    return getConstWidget(0) ? getConstWidget(0)->width() : 1;
}


int uiObject::height() const
{
    return getConstWidget(0) ? getConstWidget(0)->height() : 1;
}


int uiObject::iconSize()
{
    if ( iconsz_ < 0 )
    {
	const BufferString key =
	    IOPar::compKey( SettingsAccess::sKeyIcons(), "size" );
	iconsz_ = 32;
	Settings::common().get( key, iconsz_ );
    }

    return iconsz_;
}


int uiObject::baseFldSize()
{
    if ( fldsz_ < 0 )
    {
	fldsz_ = 10;
	Settings::common().get( "dTect.Field.size", fldsz_ );
    }

    return fldsz_;
}


void uiObject::updateToolTips()
{
    for ( int idx=uiobjectlist_.size()-1; idx>=0; idx-- )
	uiobjectlist_[idx]->updateToolTip();
}


void uiObject::reParent( uiParent* p )
{
    if ( !p ) return;

    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
	getWidget(idx)->setParent( p->getParentWidget() );
    }
}


bool uiObject::handleLongTabletPress()
{
    return false;
    /*
    if ( !parent() || !parent()->getMainObject() || parent()->getMainObject()==this )
	return false;

    return parent()->getMainObject()->handleLongTabletPress();
     */
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/

#include "uicombobox.h"
#include "uiicon.h"
#include "uilabel.h"
#include "uipixmap.h"
#include "uieventfilter.h"
#include "uivirtualkeyboard.h"
#include "uifont.h"

#include "datainpspec.h"
#include "mouseevent.h"

#include "i_qcombobox.h"

#include <QAbstractItemView>
#include <QContextMenuEvent>
#include <QSize>

mUseQtnamespace


//------------------------------------------------------------------------------


uiComboBox::uiComboBox( uiParent* parnt, const char* nm )
    : uiSingleWidgetObject( parnt, nm )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
    , messenger_( 0 )
    , combobox_( 0 )
{
    init();
}


uiComboBox::uiComboBox( uiParent* parnt, const BufferStringSet& uids,
			const char* nm )
    : uiSingleWidgetObject( parnt, nm )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
    , messenger_( 0 )
    , combobox_( 0 )
{
    init();
    addItems( uids );
}


uiComboBox::uiComboBox( uiParent* parnt, const uiStringSet& strings,
		       const char* nm )
    : uiSingleWidgetObject( parnt, nm )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
    , messenger_( 0 )
    , combobox_( 0 )
{
    init();
    addItems( strings );
}


uiComboBox::uiComboBox( uiParent* parnt, const char** uids, const char* nm )
    : uiSingleWidgetObject( parnt, nm )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
    , messenger_( 0 )
    , combobox_( 0 )
{
    init();
    addItems( uids );
}


uiComboBox::uiComboBox( uiParent* parnt, const uiString* strings,
			const char* nm )
    : uiSingleWidgetObject( parnt, nm )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
    , messenger_( 0 )
    , combobox_( 0 )
{
    init();
    for ( int idx=0; !strings[idx].isEmpty(); idx++ )
	addItem( strings[idx] );
}


uiComboBox::uiComboBox( uiParent* parnt, const EnumDef& enums,
			const char* nm )
    : uiSingleWidgetObject( parnt, nm )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(&enums)
    , messenger_( 0 )
    , combobox_( 0 )
{
    init();
    for ( int idx=0; idx<enums.size(); idx++ )
    {
	addItem( enums.getUiStringForIndex(idx), idx );
	if ( enums.getIconFileForIndex(idx) )
	{
	    setIcon( idx, enums.getIconFileForIndex(idx) );
	}
    }

    setReadOnly( true );
}


void uiComboBox::init()
{
    combobox_ = new QComboBox( getParentWidget( parent() ) );

    mAttachCB( eventFilter()->eventhappened, uiComboBox::contextMenuEventCB);
    eventFilter()->addEventType( uiEventFilter::ContextMenu );
    
    messenger_ = new i_comboMessenger( combobox_, this );
    
    setStretch( 1, 0 );
    setHSzPol( uiObject::Medium );
}


uiComboBox::~uiComboBox()
{
    detachAllNotifiers();
    delete messenger_;
}


void uiComboBox::adjustWidth( const uiString& txt )
{
    const uiFont& controlfont =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    const int txtwidth = controlfont.width( txt );

    curwidth_ = curwidth_ >= txtwidth ? curwidth_ : txtwidth;
    combobox_->view()->setMinimumWidth( curwidth_ + 50 );
}


int uiComboBox::currentItem() const
{ return combobox_->currentIndex(); }


int uiComboBox::indexOf( const char* str ) const
{
    const FixedString inputstr( str );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( inputstr == textOfItem(idx) )
	    return idx;
    }

    return -1;
}


void uiComboBox::setPixmap( int index, const uiPixmap& pixmap )
{
    if ( index >= 0 && index < combobox_->count() )
    {
	combobox_->setItemText( index, itemstrings_[index].getQString() );
	combobox_->setItemIcon( index, *pixmap.qpixmap() );
    }
}


void uiComboBox::setIcon( int index, const char* iconnm )
{
    if ( index<0 || index>=combobox_->count() )
	return;

    uiIcon icon( iconnm );
    combobox_->setItemIcon( index, icon.qicon() );
}


void uiComboBox::setEmpty()
{
    mBlockCmdRec;
    combobox_->QComboBox::clear();
    combobox_->clearEditText();
    itemids_.erase();
    itemstrings_.erase();
}


const char* uiComboBox::text() const
{
    if ( isReadOnly() )
	rettxt_ = textOfItem( currentItem() );
    else
	rettxt_ = combobox_->currentText();

    return rettxt_.buf();
}


void uiComboBox::setText( const char* txt )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);
    if ( isPresent(txt) )
	setCurrentItem(txt);
    else
    {
	bool iseditable = combobox_->isEditable();
	if ( !iseditable ) combobox_->setEditable( true );
	combobox_->setEditText( txt ? txt : "" );
	if ( !iseditable ) combobox_->setEditable( false );
    }
}


void uiComboBox::contextMenuEventCB(CallBacker * cb)
{
    mDynamicCastGet(const QContextMenuEvent*, ev,
                    eventFilter()->getCurrentEvent() );
    if ( !ev )
        return;
    
    if ( uiVirtualKeyboard::isVirtualKeyboardEnabled() )
        popupVirtualKeyboard( ev->globalX(), ev->globalY() );
}


bool uiComboBox::isPresent( const char* txt ) const
{
    return indexOf( txt ) >= 0;
}


const char* uiComboBox::textOfItem( int idx ) const
{
    if ( idx < 0 || idx >= combobox_->count() ) return sKey::EmptyString();

    if ( isReadOnly() && enumdef_ && idx<enumdef_->size() )
	return enumdef_->getKeyForIndex( idx );

    if ( itemstrings_.validIdx(idx) && (isReadOnly() ||
	 combobox_->itemText(idx)==itemstrings_[idx].getQString()) )
    {
	rettxt_ = itemstrings_[idx].getFullString();
    }
    else
    {
	rettxt_ = combobox_->itemText(idx);
    }

    return rettxt_.buf();
}


int uiComboBox::size() const
{ return combobox_->count(); }


void uiComboBox::setCurrentItem( const char* txt )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);

    const int sz = combobox_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( FixedString(textOfItem(idx)) == txt )
	    { combobox_->setCurrentIndex( idx ); return; }
    }
}


void uiComboBox::setCurrentItem( int idx )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);

    if ( idx>=0 && idx<combobox_->count() )
	combobox_->setCurrentIndex( idx );
}


void uiComboBox::setItemText( int idx, const uiString& txt )
{
    if ( idx >= 0 && idx < combobox_->count() )
    {
	adjustWidth( txt );
	combobox_->setItemText( idx, txt.getQString() );
	itemstrings_[idx] = txt;
    }
}


bool uiComboBox::update_( const DataInpSpec& spec )
{
    mDynamicCastGet(const StringListInpSpec*,spc,&spec)
    if ( !spc ) { return false; }

    setEmpty();
    int cursel = spc->getIntValue();
    if ( cursel >= 0 && cursel < spc->strings().size() )
    {
	addItems( spc->strings() );
	setCurrentItem( cursel );
	return true;
    }
    return false;
}


void uiComboBox::setReadOnly( bool yn )
{ combobox_->setEditable( !yn ); }


bool uiComboBox::isReadOnly() const
{ return !combobox_->isEditable(); }


void uiComboBox::addItem( const uiString& str )
{ addItem( str, -1 ); }


void uiComboBox::addItem( const uiString& txt, int id )
{
    mBlockCmdRec;
    adjustWidth( txt );
    combobox_->addItem( txt.getQString() );
    itemids_ += id;
    itemstrings_ += txt;
}


void uiComboBox::addItems( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	addItem( toUiString( bss.get(idx).str() ) );
}


void uiComboBox::addItems( const uiStringSet& items )
{
    for ( int idx=0; idx<items.size(); idx++ )
	addItem( items[idx] );
}


void uiComboBox::addSeparator()
{
    combobox_->insertSeparator( size() );
    itemids_ += -1;
    itemstrings_ += uiString::emptyString();
}


void uiComboBox::insertItem( const uiString& txt, int index, int id )
{
    mBlockCmdRec;
    adjustWidth( txt );
    combobox_->insertItem( index, txt.getQString() );
    itemids_.insert( index, id );
    itemstrings_.insert( index, txt );
}


void uiComboBox::insertItem( const uiPixmap& pm, const uiString& txt,
			     int index, int id )
{
    mBlockCmdRec;
    adjustWidth( txt );
    combobox_->insertItem( index, *pm.qpixmap(), txt.getQString() );
    itemids_.insert( index, id );
    itemstrings_.insert( index, txt );
}


void uiComboBox::setItemID( int index, int id )
{ if ( itemids_.validIdx(index) ) itemids_[index] = id; }

int uiComboBox::currentItemID() const
{ return getItemID(currentItem()); }

int uiComboBox::getItemID( int index ) const
{ return itemids_.validIdx(index) ? itemids_[index] : -1; }

int uiComboBox::getItemIndex( int id ) const
{ return itemids_.indexOf( id ); }


void uiComboBox::getItems( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( textOfItem( idx ) );
}


void uiComboBox::notifyHandler( bool selectionchanged )
{
    BufferString msg; msg.add( oldnritems_ );
    msg += " "; msg.add( oldcuritem_ );
    oldnritems_ = size();
    oldcuritem_ = currentItem();

    msg += selectionchanged ? " selectionChanged" : " editTextChanged";
    const int refnr = beginCmdRecEvent( msg );

    if ( selectionchanged )
	selectionChanged.trigger( this );
    else
	editTextChanged.trigger( this );

    endCmdRecEvent( refnr, msg );
}


bool uiComboBox::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x, pos.y );
    return true;
}


void uiComboBox::popupVirtualKeyboard( int globalx, int globaly )
{
    if ( isReadOnly() || !hasFocus() )
	return;

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
    {
	const char* txt = text();
	if ( !isPresent(txt) )
	    addItem( toUiString(txt) );

	setCurrentItem( txt );
	selectionChanged.trigger();
    }
}


void uiComboBox::translateText()
{
    uiObject::translateText();

    if ( !isReadOnly() )
	return;

    for ( int idx=0; idx<size(); idx++ )
    {
	combobox_->setItemText( idx, itemstrings_[idx].getQString() );
    }
}


//------------------------------------------------------------------------------


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const uiString& txt,
				      const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, nm && *nm ? nm : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const BufferStringSet& strs,
				      const uiString& txt, const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
	    ? nm
	    : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const char** strs,
				      const uiString& txt, const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
	    ? nm
	    : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const uiStringSet& strs,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
			 ? nm
			 : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const EnumDef& strs,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
			 ? nm
			 : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}

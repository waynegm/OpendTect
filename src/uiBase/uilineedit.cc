/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/

#include "uilineedit.h"
#include "i_qlineedit.h"

#include "datainpspec.h"
#include "uieventfilter.h"
#include "mouseevent.h"
#include "uivirtualkeyboard.h"

#include <QSize>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QIntValidator>
#include <QDoubleValidator>

mUseQtnamespace


//------------------------------------------------------------------------------


uiLineEdit::uiLineEdit( uiParent* parnt, const DataInpSpec& spec,
			const char* nm )
    : uiSingleWidgetObject(parnt, nm )
    , editingFinished(this), returnPressed(this)
    , selectionChanged(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    init();
    setText( spec.text() );
}


uiLineEdit::uiLineEdit( uiParent* parnt, const char* nm )
    : uiSingleWidgetObject(parnt, nm )
    , editingFinished(this), returnPressed(this)
    , selectionChanged(this), textChanged(this)
    , UserInputObjImpl<const char*>()
    , lineedit_( new QLineEdit )
{
    init();
    setText( "" );
}


uiLineEdit::~uiLineEdit()
{
    delete messenger_;
}

void uiLineEdit::init()
{
    messenger_ = new i_lineEditMessenger(lineedit_,this);
    setSingleWidget( lineedit_ );
    
    mAttachCB( eventFilter()->eventhappened, uiLineEdit::contextMenuEventCB);
    eventFilter()->addEventType( uiEventFilter::ContextMenu );
}


const char* uiLineEdit::getvalue_() const
{
    result_.set( lineedit_->text() ).trimBlanks();
    return result_.buf();
}


void uiLineEdit::setvalue_( const char* t )
{
    mBlockCmdRec;
    lineedit_->setText( mIsUdf(t) ? QString() : QString(t) );
    lineedit_->setCursorPosition( 0 );
    setEdited( false );
}


void uiLineEdit::setPasswordMode()
{
    lineedit_->setEchoMode( QLineEdit::Password );
}


void uiLineEdit::setValidator( const uiIntValidator& val )
{
    lineedit_->setValidator( new QIntValidator(val.bottom_, val.top_,lineedit_) );
}


void uiLineEdit::setValidator( const uiFloatValidator& val )
{
    QDoubleValidator* qdval =
	new QDoubleValidator( val.bottom_, val.top_, val.nrdecimals_, lineedit_ );
    if ( !val.scnotation_ )
	qdval->setNotation( QDoubleValidator::StandardNotation );
    lineedit_->setValidator( qdval );
}


void uiLineEdit::setMaxLength( int maxtxtlength )
{ lineedit_->setMaxLength( maxtxtlength ); }

int uiLineEdit::maxLength() const
{ return lineedit_->maxLength(); }

void uiLineEdit::setEdited( bool yn )
{ lineedit_->setModified( yn ); }

bool uiLineEdit::isEdited() const
{ return lineedit_->isModified(); }

void uiLineEdit::setCompleter( const BufferStringSet& bs, bool cs )
{
    QStringList qsl;
    for ( int idx=0; idx<bs.size(); idx++ )
	qsl << QString( bs.get(idx) );

    QCompleter* qc = new QCompleter( qsl, 0 );
    qc->setCaseSensitivity( cs ? Qt::CaseSensitive
			       : Qt::CaseInsensitive );
    lineedit_->setCompleter( qc );
}


void uiLineEdit::setPlaceholderText( const uiString& txt )
{
    lineedit_->setPlaceholderText( txt.getQString() );
}


void uiLineEdit::setReadOnly( bool yn )
{ lineedit_->setReadOnly( yn ); }

bool uiLineEdit::isReadOnly() const
{ return lineedit_->isReadOnly(); }

bool uiLineEdit::update_( const DataInpSpec& spec )
{ setText( spec.text() ); return true; }

void uiLineEdit::home()
{ lineedit_->home( false ); }

void uiLineEdit::end()
{ lineedit_->end( false ); }

void uiLineEdit::backspace()
{ lineedit_->backspace(); }


void uiLineEdit::contextMenuEventCB(CallBacker* cb)
{
    mDynamicCastGet(const QContextMenuEvent*, ev,
                    eventFilter()->getCurrentEvent() );
    if ( !ev )
        return;

     if ( uiVirtualKeyboard::isVirtualKeyboardEnabled() )
         popupVirtualKeyboard( ev->globalX(), ev->globalY() );
}

void uiLineEdit::del()
{ lineedit_->del(); }

void uiLineEdit::cursorBackward( bool mark, int steps )
{ lineedit_->cursorBackward( mark, steps ); }

void uiLineEdit::cursorForward( bool mark, int steps )
{ lineedit_->cursorForward( mark, steps ); }

int uiLineEdit::cursorPosition() const
{ return lineedit_->cursorPosition(); }

void uiLineEdit::insert( const char* txt )
{
    mBlockCmdRec;
    lineedit_->insert( txt );
}

int uiLineEdit::selectionStart() const
{ return lineedit_->selectionStart(); }

void uiLineEdit::setSelection( int start, int length )
{ lineedit_->setSelection( start, length ); }


const char* uiLineEdit::selectedText() const
{
    result_ = lineedit_->selectedText();
    return result_.buf();
}


bool uiLineEdit::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x, pos.y );
    return true;
}


void uiLineEdit::popupVirtualKeyboard( int globalx, int globaly )
{
    mDynamicCastGet( uiVirtualKeyboard*, virkeyboardparent, parent() );

    if ( virkeyboardparent || isReadOnly() || !hasFocus() )
	return;

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
	returnPressed.trigger();

    editingFinished.trigger();
}

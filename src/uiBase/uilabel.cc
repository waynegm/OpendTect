/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/


#include "uilabel.h"

#include "bufstringset.h"
#include "uipixmap.h"
#include "uistring.h"
#include "uifont.h"

#include "perthreadrepos.h"

#include <QLabel>

mUseQtnamespace

uiLabel::uiLabel( uiParent* p, const uiString& txt )
    : uiSingleWidgetObject(p,txt.getOriginalString())
    , qlabel_(new QLabel)
    , isrequired_(false)
{
    init( txt, 0 );
}


uiLabel::uiLabel( uiParent* p, const uiString& txt, uiObject* buddy )
    : uiSingleWidgetObject(p,txt.getOriginalString())
    , qlabel_(new QLabel)
    , isrequired_(false)
{
    init( txt, buddy );
}


void uiLabel::init( const uiString& txt, uiObject* buddy )
{
    setSingleWidget( qlabel_ );
    
    //Overcome QMacStyles setting of fonts, which is not inline with
    //our layout.
#ifdef __mac__
    setFont( uiFontList::getInst().get(FontData::Control) );
#endif
    setText( txt );
    setTextSelectable( true );

    const QString& qstr = txt.getQString();
    const int nrnewlines = qstr.count( "\n" );
    if ( nrnewlines>0 )
	setPrefHeightInChar( nrnewlines+1 );

    if ( buddy )
    {
	if ( buddy->getNrWidgets()!=1 )
	{
	    pErrMsg("Cannot buddy an object with more than one widget");

	    qlabel_->setBuddy( buddy->getWidget(0) );
	}

	buddy->attach( rightOf, this );
    }

    setStretch( 0, 0 );
}


static void addRequiredChar( QString& qstr )
{
    qstr.insert( 0, "<font color='red'><sup>*</sup></font>" );
}


void uiLabel::updateWidth()
{
    BufferStringSet strs; strs.unCat( text_.getFullString().buf() );
    if ( strs.size() != 1 )
	return;

    int lblwidth = qlabel_->fontMetrics().width( text_.getQString() ) + 1;
    if ( isrequired_ )
	lblwidth++;

    const int prefwidth = prefHNrPics();
    setPrefWidth( mMAX(lblwidth,prefwidth) );
}


void uiLabel::setText( const uiString& txt )
{
    text_ = txt;
    QString qstr = text_.getQString();
    if ( isrequired_ ) addRequiredChar( qstr );
    qlabel_->setText( qstr );
    updateWidth();
    setName( text_.getOriginalString() );
}


void uiLabel::makeRequired( bool yn )
{
    isrequired_ = yn;
    QString qstr = text_.getQString();
    if ( qstr.isEmpty() ) return;

    if ( isrequired_ ) addRequiredChar( qstr );
    qlabel_->setText( qstr );
    updateWidth();
}


void uiLabel::translateText()
{
    uiObject::translateText();
    QString qstr = text_.getQString();
    if ( isrequired_ ) addRequiredChar( qstr );
    qlabel_->setText( qstr );
    updateWidth();
}


const uiString& uiLabel::text() const
{
    return text_;
}


void uiLabel::setTextSelectable( bool yn )
{
    qlabel_->setTextInteractionFlags( yn ? Qt::TextSelectableByMouse :
					 Qt::NoTextInteraction );
}


void uiLabel::setPixmap( const uiPixmap& pixmap )
{
    if ( !pixmap.qpixmap() ) return;

    const uiFont& ft =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    const QPixmap pm = pixmap.qpixmap()->scaledToHeight( ft.height() );
    qlabel_->setPixmap( pm );
    qlabel_->setAlignment( Qt::AlignCenter );
}


void uiLabel::setAlignment( OD::Alignment::HPos hal )
{
    OD::Alignment al( hal, OD::Alignment::VCenter );
    qlabel_->setAlignment( (Qt::AlignmentFlag)al.uiValue() );
}


OD::Alignment::HPos uiLabel::alignment() const
{
    OD::Alignment al;
    al.setUiValue( (int)qlabel_->alignment() );
    return al.hPos();
}

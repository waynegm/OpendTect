/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "uispinbox.h"

#include "datainpspec.h"
#include "ranges.h"
#include "scaler.h"

#include <QString>
#include <math.h>

mUseQtnamespace


// uiSliderObj
uiSliderObj::uiSliderObj( uiParent* p, const char* nm )
    : uiSingleWidgetObject(p,nm)
    , slider_( new QSlider )
    , messenger_( 0 )
    , valueChanged(this)
    , sliderMoved(this)
    , sliderPressed(this)
    , sliderReleased(this)
{
    messenger_ = new i_SliderMessenger(slider_,this);
    setSingleWidget( slider_ );
    slider_->setFocusPolicy( Qt::WheelFocus );
}

uiSliderObj::~uiSliderObj()
{ delete messenger_; }


void uiSliderObj::setOrientation( OD::Orientation orient )
{
    slider_->setOrientation(
                orient == OD::Vertical
                            ? Qt::Vertical
                            : Qt::Horizontal );
}

float uiSliderObj::getLinearFraction() const
{
    float start = slider_->minimum();
    float range = slider_->maximum() - start;
    return range ? (slider_->sliderPosition()-start)/range : 1.0;
}


void uiSliderObj::setLinearFraction( float frac )
{
    mSliderBlockCmdRec;
    if ( frac>=0.0 && frac<=1.0 )
    {
        const float val = (1-frac)*slider_->minimum() +
        frac*slider_->maximum();
        slider_->setValue( mNINT32(val) );
    }
}


int uiSliderObj::getIntValue() const
{ return slider_->value(); }


void uiSliderObj::setTickMarks( TickPosition ticks )
{ slider_->setTickPosition( QSlider::TickPosition( (int)ticks ) ); }

uiSlider::TickPosition uiSliderObj::tickMarks() const
{ return (uiSlider::TickPosition)( (int)slider_->tickPosition() ); }


OD::Orientation uiSliderObj::getOrientation() const
{ return (OD::Orientation)( (int)slider_->orientation() ); }

void uiSlider::setInverted( bool yn )
{ slider_->setInvertedAppearance( yn ); }

bool uiSlider::isInverted() const
{ return slider_->invertedAppearance(); }

void uiSlider::setInvertedControls( bool yn )
{ slider_->body().setInvertedControls( yn ); }

bool uiSlider::hasInvertedControls() const
{ return slider_->body().invertedControls(); }


void uiSlider::setMinValue( float minval )
{
    mSliderBlockCmdRec;
    slider_->body().setMinimum( sliderValue(minval) );
}


void uiSlider::setMaxValue( float maxval )
{
    mSliderBlockCmdRec;
    slider_->body().setMaximum( sliderValue(maxval) );
}


float uiSliderObj::minValue() const
{
    return userValue( slider_->minimum() );
}


float uiSliderObj::maxValue() const
{
    return userValue( slider_->maximum() );
}


//------------------------------------------------------------------------------

uiSlider::uiSlider( uiParent* p, const Setup& setup, const char* nm )
    : uiGroup(p,nm)
    , lbl_(0)
    , editfld_(0)
    , inteditfld_(0)
    , logscale_(setup.logscale_)
{
    init( setup, nm );
}


uiSlider::~uiSlider()
{
    delete scaler_;
}


void uiSlider::init( const uiSlider::Setup& setup, const char* nm )
{
    slider_ = new uiSliderObj( this, nm );
    const bool isvert = setup.isvertical_;

    slider_->setOrientation( isvert ? OD::Vertical : OD::Horizontal );
    //slider_->body().setStretch( isvert ? 0 : 1, isvert ? 1 : 0 );

    int nrdec = setup.nrdec_;
    if ( nrdec < 0 ) nrdec = 0;
    double factor = pow( 10., -nrdec );
    scaler_ = new LinScaler( 0, factor );

    if ( !setup.lbl_.isEmpty() )
	lbl_ = new uiLabel( this, setup.lbl_ );

    if ( setup.withedit_ )
    {
        mAttachCB( slider_->valueChanged, uiSlider::sliderMoveCB );
	editfld_ = new uiLineEdit( this,
			BufferString(setup.lbl_.getFullString()," value") );
	editfld_->setHSzPol( uiObject::Small );
        mAttachCB( editfld_->returnPressed, uiSlider::editRetPressCB );
	sliderMoveCB(0);
    }

    if ( setup.isvertical_ )
    {
	slider_->setPrefHeight( setup.sldrsize_ );
	slider_->setPrefWidth( 10 );
	if ( lbl_ ) slider_->attach( centeredBelow, lbl_ );
	if ( editfld_ ) editfld_->attach( centeredBelow, slider_ );
    }
    else
    {
	slider_->setPrefWidth( setup.sldrsize_ );
	if ( lbl_ ) slider_->attach( rightOf, lbl_ );
	if ( editfld_ ) editfld_->attach( rightOf, slider_ );
    }

    setInverted( setup.isinverted_ );
    setInvertedControls( setup.isinverted_ );

    setHAlignObj( slider_ );
}


#define mSliderBlockCmdRec	CmdRecStopper cmdrecstopper( slider_ );


void uiSlider::processInput()
{ if ( editfld_ ) setValue( editfld_->getFValue() ); }

void uiSlider::setToolTip( const uiString& tt )
{ slider_->setToolTip( tt ); }


float uiSlider::getLinearFraction() const
{
    return slider_->getLinearFraction();
}


void uiSlider::setScale( float fact, float constant )
{
    const float userval = getFValue();
    scaler_->factor = fact;
    scaler_->constant = constant;
    setValue( userval );
}


int uiSlider::sliderValue( float fval ) const
{
    if ( logscale_ )
    {
	if ( fval <= 0 ) return 0;
	fval = log10( fval );
    }

    return mNINT32( scaler_ ? scaler_->unScale(fval) : fval );
}


float uiSlider::userValue( int ival ) const
{
    double res = scaler_->scale( (double)ival );
    return logscale_ ? pow( 10, res ) : res;
}


void uiSlider::setText( const char* txt )
{ setValue( toFloat(txt) ); }

void uiSlider::setValue( int ival )
{ slider_->setValue( ival ); }

void uiSlider::setValue( float fval )
{
    mSliderBlockCmdRec;
    int val = sliderValue( fval );
    slider_->setValue( val );
}


const char* uiSlider::text() const
{
    result_ = userValue( getIntValue() );
    return (const char*)result_;
}


int uiSlider::getIntValue() const
{ return slider_->getIntValue(); }


float uiSlider::getFValue() const
{ return userValue( getIntValue() );



void uiSlider::setStep( float stp )
{
    mSliderBlockCmdRec;
    int istep = (int)stp;
    if ( scaler_ )
    {
	const float fstp = stp / scaler_->factor;
	istep = mNINT32( fstp );
    }
    slider_->body().setSingleStep( istep );
    slider_->body().setPageStep( istep );
}


float uiSlider::step() const
{
    return userValue( slider_->body().singleStep() );
}


void uiSlider::setInterval( const StepInterval<int>& intv )
{ setInterval( intv.start, intv.stop, intv.step ); }

void uiSlider::setInterval( int start, int stop, int stp )
{
    slider_->body().setRange( start, stop );
    slider_->body().setSingleStep( stp );
    slider_->body().setPageStep( stp );
}


void uiSlider::setInterval( const StepInterval<float>& intv )
{ setInterval( intv.start, intv.stop, intv.step ); }

void uiSlider::setInterval( float start, float stop, float stp )
{
    setMinValue( start );
    setMaxValue( stop );
    setStep( stp );
}


void uiSlider::getInterval( StepInterval<float>& intv ) const
{
    intv.start = minValue();
    intv.stop = maxValue();
    intv.step = step();
}


void uiSlider::setLinearScale( double constant, double factor )
{
    if ( scaler_ )
    {
	scaler_->constant = constant;
	scaler_->factor =  factor;
    }
}


int uiSlider::tickStep() const
{ return slider_->body().tickInterval() ; }

void uiSlider::setTickStep( int s )
{ slider_->body().setTickInterval(s); }


void uiSlider::sliderMove( CallBacker* )
{
    const float val = getFValue();
    if ( editfld_ ) editfld_->setValue( val );
}


float uiSlider::editValue() const
{
    return editfld_ ? editfld_->getFValue() : mUdf(float);
}

void uiSlider::editRetPress( CallBacker* )
{
    if ( editfld_ )
	setValue( editfld_->getFValue() );

    valueChanged.trigger();
}

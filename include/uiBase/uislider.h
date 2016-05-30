#ifndef uislider_h
#define uislider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "uiobj.h"
#include "uistring.h"

class LinScaler;

class uiLabel;
class uiLineEdit;
class uiSliderBody;
class uiSpinBox;
class i_SliderMessenger;
mFDQtclass(QSlider);


mExpClass(uiBase) uiSliderObj : public uiSingleWidgetObject
{
public:
				uiSliderObj(uiParent*,const char* nm);
				~uiSliderObj();
    
    void			setVertical(bool isvert);
    bool			isVertical() const;
    
    void			setInverted( bool yn );
    bool			isInverted() const;
    
    void			setInvertedControls( bool yn );
    bool			hasInvertedControls() const;

    int				getSliderPosition() const;
    void			setSliderPosition(int);
    
    int				getMinimum() const;
    void			setMinimum(int);
    
    int 			getMaximum() const;
    void			setMaximum(int);
    
    int				getStep() const;
    void			setStep(int);
    
    enum			TickPosition { NoMarks=0, Above=1, Left=Above,
        					Below=2, Right=Below, Both=3 };
    
    void			setTickMarks(TickPosition);
    TickPosition		getTickMarks() const;
    void			setTickStep(int);
    int				getTickStep() const;
    
    Notifier<uiSliderObj>	valueChanged;
    Notifier<uiSliderObj>	sliderMoved;
    Notifier<uiSliderObj>	sliderPressed;
    Notifier<uiSliderObj>	sliderReleased;
    
private:
    i_SliderMessenger*	messenger_;
    
    QSlider*		slider_;
};


mExpClass(uiBase) uiSlider : public uiGroup
{
public:
    mExpClass(uiBase) Setup
    {
    public:
			Setup(const uiString& l=uiString::emptyString())
			    : lbl_(l)
			    , withedit_(false)
			    , nrdec_(0)
			    , logscale_(false)
			    , isvertical_(false)
			    , sldrsize_(200)
			    , isinverted_(false)
			    {}

	mDefSetupMemb(bool,withedit)
	mDefSetupMemb(bool,logscale)
	mDefSetupMemb(bool,isvertical)
	mDefSetupMemb(int,nrdec)
	mDefSetupMemb(int,sldrsize)
	mDefSetupMemb(bool,isinverted)
	mDefSetupMemb(uiString,lbl)
    };

			uiSlider(uiParent*,const Setup&,const char* nm=0);
			~uiSlider();



    void		processInput();
    void		setToolTip(const uiString&);
    void		setText(const char*);
    const char*		text() const;

    void		setValue(int);
    void		setValue(float);
    int			getIntValue() const;
    float		getFValue() const;
    float		editValue() const;

    void		setMinValue(float);
    float		minValue() const;
    void		setMaxValue(float);
    float		maxValue() const;
    void		setStep(float);
    void		setScale(float fact,float constant);
    float		step() const;

    void		setInterval(const StepInterval<int>&);
    void		setInterval(int start,int stop,int step=1);
    void		setInterval(const StepInterval<float>&);
    void		setInterval(float start,float stop,float step);
    void		getInterval(StepInterval<float>&) const;
    void		setLinearScale(double,double);

    void		setTickMarks(uiSliderObj::TickPosition);
    uiSliderObj::TickPosition	tickMarks() const;
    void		setTickStep(int);
    int			tickStep() const;

    void		setOrientation(OD::Orientation);
    OD::Orientation	getOrientation() const;

    void		setInverted(bool);
    bool		isInverted() const;
    void		setInvertedControls(bool);
    bool		hasInvertedControls() const;

    bool		isLogScale()			{ return logscale_; }

    float		getLinearFraction() const;
    void		setLinearFraction(float fraction);

    const uiLabel*	label() const			{ return lbl_; }
    uiLabel*		label()				{ return lbl_; }

    uiSliderObj*	slider()			{ return slider_; }

private:

    uiSliderObj*	slider_;
    uiLabel*		lbl_;
    uiLineEdit*		editfld_;
    uiSpinBox*		inteditfld_;

    mutable BufferString result_;
    LinScaler*		scaler_;
    bool		logscale_;

    void		init(const Setup&,const char*);

    void		sliderMoveCB(CallBacker*);
    void		editRetPressCB(CallBacker*);

    float		userValue(int) const;
    int			sliderValue(float) const;

public:
    /*mDeprecated*/ float	getValue() const	{ return getFValue(); }
};

#endif

#ifndef uibutton_h
#define uibutton_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uistring.h"
#include "odiconfile.h"

mFDQtclass(QPushButton);
mFDQtclass(QCheckBox);
mFDQtclass(QRadioButton);
mFDQtclass(QAbstractButton)

class uiMenu;
class uiPixmap;
class i_ButMessenger;
mFDQtclass(QEvent);
mFDQtclass(QMenu);


//!\brief is the base class for all buttons.

mExpClass(uiBase) uiButton : public uiSingleWidgetObject
{
public:
			uiButton(uiParent*,const uiString&,const CallBack*);
    virtual		~uiButton();

    virtual void	setText(const uiString&);
    const uiString&	text() const			{ return text_; }
    void		setIcon(const char* icon_identifier);
    void		setPixmap( const uiPixmap& pm ) { setPM(pm); }
    void		setIconScale(float val); /*!< val between [0-1] */
    virtual void	updateIconSize()	{}

    virtual void	click()		= 0;

    Notifier<uiButton>	activated;

    static uiButton*	getStd(uiParent*,OD::StdActionType,const CallBack&,
				bool immediate);
    static uiButton*	getStd(uiParent*,OD::StdActionType,const CallBack&,
				bool immediate,const uiString& nonstd_text);
				//!< will deliver toolbutton is txt is empty

    static bool		haveCommonPBIcons()	{ return havecommonpbics_; }
    static void		setHaveCommonPBIcons( bool yn=true )
						{ havecommonpbics_ = yn; }

protected:

    friend class	i_ButMessenger;
    i_ButMessenger*	messenger_;
    uiString		text_;
    float		iconscale_;
    static bool		havecommonpbics_;
    void		doTrigger();
    void		setButtonWidget(mQtclass(QAbstractButton*));

    virtual void	translateText();
    virtual void	setPM(const uiPixmap&);
    
    virtual void	toggled()	{}
    virtual void	clicked()	{}
    virtual void	pressed()	{}
    virtual void	released()	{}
private:
    
    void		updateIconCB(CallBacker*);
    void		eventCB(CallBacker*);
    
    BufferString		icon_;
    mQtclass(QAbstractButton)*	button_;
};


/*!\brief Push button. By default, assumes immediate action, not a dialog
  when pushed. The button text will in that case get an added " ..." to the
  text. In principle, it could also get another appearance.
*/

mExpClass(uiBase) uiPushButton : public uiButton
{
public:
			uiPushButton(uiParent*,const uiString& txt,
				     bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const CallBack&,bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const uiPixmap&,bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const uiPixmap&,const CallBack&,
				     bool immediate);

    void		setDefault(bool yn=true);
    void		click();
    void		setMenu(uiMenu*);
    void		setFlat(bool);

private:
    void		clicked() { doTrigger(); }
    			//Pushbutton is activated on click

    void		translateText();
    void		updateText();
    void		updateIconSize();

    bool			immediate_;
    mQtclass(QPushButton)*	pbbody_;
};


mExpClass(uiBase) uiRadioButton : public uiButton
{
public:
			uiRadioButton(uiParent*,const uiString&);
			uiRadioButton(uiParent*,const uiString&,
				      const CallBack&);

    bool		isChecked() const;
    virtual void	setChecked(bool yn=true);

    void		click();

private:
    
    void		clicked() { doTrigger(); }
    			//Radiobutton is activated on click

    mQtclass(QRadioButton)*	rbbody_;
};


mExpClass(uiBase) uiCheckBox: public uiButton
{
public:

			uiCheckBox(uiParent*,const uiString&);
			uiCheckBox(uiParent*,const uiString&,
				   const CallBack&);

    bool		isChecked() const;
    void		setChecked(bool yn=true);
    void		setTriState(bool yn=true);
    void		setCheckState(OD::CheckState);
    OD::CheckState	getCheckState() const;

    void		click();

private:
    void		toggled() { doTrigger(); }

    mQtclass(QCheckBox)* cbbody_;
};

#endif

#ifndef uibutton_h
#define uibutton_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uibaseobject.h"

class ioPixmap;
class i_ButMessenger;

mFDQtclass(QPushButton);
mFDQtclass(QAbstractButton);

//!\brief Button Abstract Base class
mClass(uiBase) uiButton : public uiBaseObject
{
public:
    mQtclass(QWidget)*	getWidget(int);
    
			~uiButton();
			
    virtual void	setText(const char*);
    const char*		text();

    //virtual void	click()			{}

    Notifier<uiButton>	activated;

protected:
			uiButton(uiGroup* parent, const char* nm,
			    const CallBack* cb,mQtclass(QAbstractButton)*);
    virtual void	toggled(bool)	{}
    virtual void	clicked()	{ activated.trigger(); }
    
    friend		class i_ButMessenger;
    i_ButMessenger*	messenger_;
};


/*!\brief Push button. By default, assumes immediate action, not a dialog
  when pushed. The button text will in that case get an added " ..." to the
  text. In principle, it could also get another appearance.
  */

mClass(uiBase) uiPushButton : public uiButton
{
public:
				uiPushButton(uiGroup*,const char* nm,
					     bool immediate);
				uiPushButton(uiGroup*,const char* nm,
					     const CallBack&,
					     bool immediate); 
				uiPushButton(uiGroup*,const char* nm,
					     const ioPixmap&,
					     bool immediate);
				uiPushButton(uiGroup*,const char* nm,
					     const ioPixmap&,const CallBack&,
					     bool immediate);

				~uiPushButton();

    void			setDefault(bool yn=true);
    void			setPixmap(const char*);
    void			setPixmap(const ioPixmap&);
    				//! Size of pixmap is 1/2 the size of button

    void			click();

protected:
    mQtclass(QPushButton)*	getButton();
};

/*
mClass(uiBase) uiRadioButton : public uiButton
{                        
public:
				uiRadioButton(uiParent*,const char*);
				uiRadioButton(uiParent*,const char*,
					      const CallBack&);

    bool			isChecked() const;
    virtual void		setChecked(bool yn=true);

    void			click();

private:

    uiRadioButtonBody*		body_;
    uiRadioButtonBody&		mkbody(uiParent*,const char*);

};


mClass(uiBase) uiCheckBox: public uiButton
{
public:

				uiCheckBox(uiParent*,const char*);
				uiCheckBox(uiParent*,const char*,
					   const CallBack&);

    bool			isChecked() const;
    void			setChecked(bool yn=true);

    void			click();

    virtual void		setText(const char*);

private:

    uiCheckBoxBody*		body_;
    uiCheckBoxBody&		mkbody(uiParent*,const char*);

};


//! Button Abstract Base class
mClass(uiBase) uiButtonBody
{
    friend class        i_ButMessenger;

public:
			uiButtonBody()				{}
    virtual		~uiButtonBody()				{}

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled };
    
protected:

    //! Handler called from Qt.
    virtual void        notifyHandler(notifyTp)			=0;
};

*/


#endif


#ifndef uistratseisevent_h
#define uistratseisevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "stratseisevent.h"
class uiGenInput;
class uiStratLevelSel;


mExpClass(uiWellAttrib) uiStratSeisEvent : public uiGroup
{
public:

    mExpClass(uiWellAttrib) Setup
    {
    public:
			Setup( bool wew=false )
			    : withextrwin_(wew)
			    , fixedlevel_(0)		{}

	mDefSetupMemb(const Strat::Level*,fixedlevel)
	mDefSetupMemb(bool,withextrwin)
    };

    			uiStratSeisEvent(uiParent*,const Setup&);

    bool		getFromScreen();
    void		setLevel(const char* lvlnm);
    void		putToScreen();
    const char*		levelName() const;

    Strat::SeisEvent&	event()		{ return ev_; }

protected:

    Strat::SeisEvent	ev_;
    Setup		setup_;

    uiStratLevelSel*	levelfld_;
    uiGenInput*		evfld_;
    uiGenInput*		snapoffsfld_;
    uiGenInput*		extrwinfld_;

    void		evSnapCheck(CallBacker*);

};


#endif


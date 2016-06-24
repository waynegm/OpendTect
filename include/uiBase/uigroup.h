#ifndef uigroup_h
#define uigroup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uiparent.h"
#include "notify.h"


class uiGroupBody;
class uiParentBody;

class uiGroup;
class uiGroupObjBody;
class uiGroupParentBody;
class uiLayoutMgr;

mFDQtclass(QFrame);


mExpClass(uiBase) uiGroup : public uiParent, public uiObject
{
friend class		uiMainWin;
friend class		uiTabStack;
public:
			uiGroup( uiParent* , const char* nm="uiGroup",
				 bool manage=true );
    virtual		~uiGroup();

    virtual void	setSize(const uiSize&);
    void		setHSpacing( int )	{}
    void		setVSpacing( int )	{}
    void		setSpacing( int s=0 )
			{ setHSpacing(s); setVSpacing(s); }
    void		setBorder( int )	{}

    static uiGroup*	gtDynamicCastToGrp( mQtclass(QWidget*) );

    void		setSensitive(bool);

    void		finalise();
    void		translateText();
    
    uiLayoutMgr*	getLayoutMgr() 	{ return layoutmgr_; }

    virtual int			getNrWidgets() const;
    virtual mQtclass(QWidget*)	getWidget(int idx);

    mDeprecated const uiObject*	hAlignObj() { return getHAlignObj(); } 
    mDeprecated const uiObject*	hCenterObj() { return getHCenterObj(); } 

protected:

    uiLayoutMgr*        layoutmgr_;
    
    virtual const uiObject*	getHAlignObj() const;
    virtual const uiObject*	getHCenterObj() const;
    virtual void		setHAlignObj(const uiObject*);
    virtual void		setHCenterObj(const uiObject*);

    void		reSizeChildren(const uiObject*,float,float);

};


mExpClass(uiBase) uiLayoutGroup : public uiGroup
{
public:
				uiLayoutGroup(uiParent*,
					      const char* nm="uiLayoutGroup");
    virtual int			getNrWidgets() const	{ return 1; }
    virtual mQtclass(QWidget*)	getWidget(int idx);
    void			setFrame( bool yn=true );
    void			setNoBackGround();

    mQtclass(QWidget*)		getParentWidget() 		{ return getWidget(0); }



protected:
    virtual const uiObject*	getHAlignObj() const		{ return 0; }
    virtual const uiObject*	getHCenterObj() const		{ return 0; }
    virtual void		setHAlignObj(const uiObject*);
    virtual void		setHCenterObj(const uiObject*);
    void			setFrameStyle(int);

    void			setShrinkAllowed(bool);
    bool			shrinkAllowed();

    mQtclass(QFrame*)		widget_;
};


#endif

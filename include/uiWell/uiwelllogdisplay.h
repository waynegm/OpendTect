#ifndef uiwelllogdisplay_h
#define uiwelllogdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.27 2010-04-08 13:13:11 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uimainwin.h"
#include "uigroup.h"
#include "uiaxishandler.h"
#include "draw.h"
#include "welldata.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"

class uiGraphicsScene;
class uiLineItem;
class uiMenuHandler;
class uiPolyLineItem;
class uiPolygonItem;
class uiTextItem;
class uiWellStratDisplay;
class uiWellDisplayMarkerEdit;

class UnitOfMeasure;
namespace Well 
{ 
    class Log; 
    class D2TModel; 
    class Track; 
    class DahObj;
}

#define mLogWidth 170
#define mLogHeight 600

/*!\brief creates a display of max 2 well logs. */
mClass uiWellLogDisplay : public uiGraphicsView
{
public:

    mStruct Setup
    {
				Setup()
				    : nrmarkerchars_(2)
				    , markerls_(LineStyle::Dot,1)
				    , pickls_(LineStyle::Solid,1,Color(0,200,0))
				    , noxgridline_(false)
				    , noygridline_(false)
				    , border_(5)
				    , yborder_(5)
				    , noborderspace_(false)
				    , axisticsz_(2)			   
				    , noxpixbefore_(false)
				    , noypixbefore_(false)
				    , noxpixafter_(false)
				    , noypixafter_(false)
				    , noxaxisline_(false)
				    , noyaxisline_(false)
				    , nobackground_(false)		 
				    {}

	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(uiBorder,yborder)
	mDefSetupMemb(int,nrmarkerchars)  //!< Will display up to this nr chars
	mDefSetupMemb(LineStyle,markerls) //!< will not use color
	mDefSetupMemb(LineStyle,pickls)	  //!< color used if no PickData color
	mDefSetupMemb(bool,noxpixbefore)
	mDefSetupMemb(bool,noypixbefore)
	mDefSetupMemb(bool,noxpixafter)
	mDefSetupMemb(bool,noypixafter)
	mDefSetupMemb(bool,noborderspace)
	mDefSetupMemb(int,axisticsz)
	mDefSetupMemb(bool,noygridline)
	mDefSetupMemb(bool,noxgridline)
	mDefSetupMemb(bool,noyaxisline)
	mDefSetupMemb(bool,noxaxisline)
	mDefSetupMemb(bool,nobackground)
    };

				uiWellLogDisplay(uiParent*,const Setup&);
				~uiWellLogDisplay(){};

    mStruct LineData
    {
	    mStruct Setup
	    {
				Setup()
				    : border_(5)
				    , noborderspace_(false)
				    , xaxisticsz_(2)			   
				    , noxgridline_(false)
				    , noygridline_(false)
				    , noxaxisline_(false)
				    , noyaxisline_(false)
				    {}

		mDefSetupMemb(uiBorder,border)
		mDefSetupMemb(bool,noborderspace)
		mDefSetupMemb(int,xaxisticsz)
		mDefSetupMemb(bool,noyaxisline)
		mDefSetupMemb(bool,noxaxisline)
		mDefSetupMemb(bool,noxgridline)
		mDefSetupMemb(bool,noygridline)
	    };

				LineData(uiGraphicsScene&,Setup);

	Interval<float>		zrg_;
	Interval<float>		valrg_;
	uiAxisHandler		xax_;
	uiAxisHandler		yax_;
	Setup 			setup_;
	
	// Get these (will be filled)
	ObjectSet<uiPolyLineItem> curveitms_;
	ObjectSet<uiPolygonItem> curvepolyitms_;
	uiTextItem*		curvenmitm_;
	Alignment 		al_;
    };

    mStruct LogData : public LineData
    {
	// Set these
	const Well::Log*	wl_;
	const UnitOfMeasure*	unitmeas_;
	bool			xrev_;
	bool			isyaxisleft_;

	Well::DisplayProperties::Log wld_;

	protected:
				LogData(uiGraphicsScene&,bool,Setup);

	void			copySetupFrom( const LogData& ld )
	    			{ unitmeas_ = ld.unitmeas_; xrev_ = ld.xrev_; }
	friend class		uiWellLogDisplay;
    };
    
    LogData&			logData(bool first=true)
    				{ return first ? ld1_ : ld2_; }
    void                        setD2TModel( const Well::D2TModel* d2tm )
				{ d2tm_ = d2tm; }
    const Interval<float>&	zRange() const	{ return zrg_; }
    void			setZRange(const Interval<float>&);
    void			dataChanged();

    void			setZDispInFeet( bool yn )
    				{ dispzinft_ = yn; dataChanged(); }
    bool			zDispInFeet() const	  { return dispzinft_; }

    void			setZInTime( bool yn )
    				{ zistime_ = yn; dataChanged(); }
    bool			zIsTime() const	  { return zistime_; }

    mStruct MarkerItem
    {
			MarkerItem( Well::Marker& mrk )
			    : mrk_(mrk) {}
			~MarkerItem()
			{
			    delete itm_; delete txtitm_;
			}
	Well::Marker&	mrk_;
	uiLineItem*	itm_;
	uiTextItem*	txtitm_;
	Color		color_;	
    };

    Notifier<uiWellLogDisplay>  highlightedMarkerItemChged;
    void 			highlightMarkerItem(const Well::Marker*);
    MarkerItem* 		getMarkerItem(const Well::Marker*);
    MarkerItem*			highlightedmrkitem_;
    ObjectSet<MarkerItem>&   	markerItems() { return  markeritms_; } 
    void			reDrawMarkers(CallBacker*);
    void			setEditMarkers(bool);
    void			setMarkers( ObjectSet<Well::Marker>* ms )
				{ markers_ = ms; }

    mStruct PickData
    {
			PickData( float dah, Color c=Color::NoColor() )
			    : dah_(dah), color_(c)	{}
	bool		operator ==( const PickData& pd ) const
	    		{ return mIsEqual(pd.dah_,dah_,1e-4); }

	float		dah_;
	Color		color_;	//!< default will use the global setup color
    };

    TypeSet<PickData>&		zPicks()	{ return zpicks_; }

protected:

    LogData			ld1_;
    LogData			ld2_;
    Interval<float>		zrg_;
    bool			zistime_;
    bool			dispzinft_;
    bool			markeredit_;

    ObjectSet<Well::Marker>* 	markers_;
    TypeSet<PickData>		zpicks_;
    const Well::D2TModel*       d2tm_;

    Setup			setup_;

    ObjectSet<MarkerItem>	markeritms_;
    ObjectSet<uiLineItem>	zpickitms_;

    void			init(CallBacker*);
    void			reSized(CallBacker*);

    void			setAxisRelations();
    void			setAxisRanges(bool);
    void			gatherInfo();
    void			gatherInfo(bool);
    void			draw();
    void			drawLog(bool);
    void			drawLine(LogData&,const Well::DahObj* ldah);
    void			drawFilling(bool);
    void			drawMarkers();
    void			drawZPicks();
};


mClass uiWellDisplay : public uiGroup
{
public:

    mStruct Setup
    {
				Setup()
				    : nrpanels_(1)
				    , noborderspace_(false)
				    , nobackground_(false)		   
				    , logwidth_(mLogWidth)	 	   
				    , logheight_(mLogHeight)
				    , withstratdisp_(false) 		    
				    {}

	mDefSetupMemb(int,nrpanels) 	 	// nr Log Panels
	mDefSetupMemb(bool,nobackground) 	//transparent background
	mDefSetupMemb(bool,noborderspace) 	//will remove all annots 
	mDefSetupMemb(int,logheight) 		//log height
	mDefSetupMemb(int,logwidth) 		//log width
	mDefSetupMemb(bool,withstratdisp) 	//Add Stratigraphy display
    };

    mClass ShapeSetup
    {
	public:
				    ShapeSetup(uiParent* p)
				    : parent_(p)
				    , withstrat_(false)
				    , nrlogpanels_(1)
				    {}
	uiParent* 	parent_;
	bool 		withstrat_;
	int 		nrlogpanels_;
    };

				uiWellDisplay(uiParent*,const Setup&,
						Well::Data&);
				uiWellDisplay(uiWellDisplay&,const ShapeSetup&);
				~uiWellDisplay();
    
   
    int 			nrLogDisp() 	{ return logdisps_.size();}	
    uiWellLogDisplay* 		logDisplay(int idx) { return logdisps_[idx]; }

    Well::Data&			wellData() 		{ return wd_; }
    const Well::Data&		wellData() const 	{ return wd_; }

    void			setLog(const char*,int,bool);
    void			setInitialZRange();
    void			setZRange(Interval<float>);
    const Interval<float>&	zRange() const	 	{ return zrg_; }
    void			setZInTime( bool yn )
    				{ zistime_ = yn; dataChanged(0); }
    bool			zInTime() const 	{ return zistime_; }
    uiWellStratDisplay*		stratDisp() 		{ return stratdisp_; }
    const uiWellStratDisplay*	stratDisp() const  	{ return stratdisp_; }
    bool			hasStratDisp() const	{ return stratdisp_; }

protected:

    Well::Data&			wd_;
    const Well::D2TModel*	d2tm_;
    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellStratDisplay*		stratdisp_;
    uiWellDisplayMarkerEdit*	mrkedit_;

    Interval<float>		zrg_;
    bool			zistime_;
    int 			logwidth_;	 	   
    int 			logheight_;
    bool			noborderspace_;
    void			addMarker();
    void			setLogPanel(bool,bool);
    void                        gatherInfo();
    void                        setAxisRanges();

    void                        dataChanged(CallBacker*);
    void 			updateProperties(CallBacker*); 
};


mClass uiWellDisplayWin : public uiMainWin
{
public:
    			uiWellDisplayWin(uiParent*,Well::Data&);
			~uiWellDisplayWin(){};

protected:

    uiWellDisplay& 	welldisp_;
    Well::Data&		wd_;
    
    void		closeWin(CallBacker*);
};

#endif

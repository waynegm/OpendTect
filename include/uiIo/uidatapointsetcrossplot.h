#ifndef uidatapointsetcrossplot_h
#define uidatapointsetcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.h,v 1.6 2008-06-25 12:18:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "uidatapointset.h"
#include "datapointset.h"
#include "uirgbarraycanvas.h"
class ioDrawTool;
class MouseEvent;
class LinStats2D;
class uiDataPointSet;

/*!\brief Data Point Set Cross Plotter */

class uiDataPointSetCrossPlotter : public uiRGBArrayCanvas
{
public:

    struct Setup
    {
			Setup();

	mDefSetupMemb(bool,noedit)
	mDefSetupMemb(uiBorder,minborder)
	mDefSetupMemb(MarkerStyle2D,markerstyle) // None => uses drawPoint
	mDefSetupMemb(LineStyle,xstyle)
	mDefSetupMemb(LineStyle,ystyle)
	mDefSetupMemb(LineStyle,y2style)
	mDefSetupMemb(bool,showcc)		// corr coefficient
	mDefSetupMemb(bool,showregrline)
    };

    			uiDataPointSetCrossPlotter(uiParent*,uiDataPointSet&,
						   const Setup&);
    			~uiDataPointSetCrossPlotter();

    const Setup&	setup() const		{ return setup_; }
    Setup&		setup()			{ return setup_; }

    void		setCols(DataPointSet::ColID x,
	    			DataPointSet::ColID y,
				DataPointSet::ColID y2);

    struct AutoScalePars
    {
			AutoScalePars();

	bool		doautoscale_;
	float		clipratio_;

	static float	defclipratio_;
			//!< 1) settings "AxisData.Clip Ratio"
			//!< 2) env "OD_DEFAULT_AXIS_CLIPRATIO"
			//!< 3) zero
    };
    AutoScalePars&	autoScalePars(int ax);		//!< 0=x 1=y 2=y2
    uiAxisHandler*	axisHandler(int ax);		//!< 0=x 1=y 2=y2
    const LinStats2D&	linStats( bool y1=true ) const
    						{ return y1 ? lsy1_ : lsy2_; }

    Notifier<uiDataPointSetCrossPlotter>	selectionChanged;
    Notifier<uiDataPointSetCrossPlotter>	removeRequest;
    DataPointSet::RowID	selRow() const		{ return selrow_; }
    bool		selRowIsY2() const	{ return selrowisy2_; }

    void		dataChanged();

protected:

    struct AxisData
    {
				AxisData(uiDataPointSetCrossPlotter&,
					 uiRect::Side);
				~AxisData();

	void			stop();
	void			start();
	void			setCol(DataPointSet::ColID);

	uiDataPointSetCrossPlotter& cp_;
	DataPointSet::ColID	colid_;
	uiAxisHandler*		axis_;
	AutoScalePars		autoscalepars_;
	Interval<float>		rg_;

	bool			needautoscale_;
	uiAxisHandler::Setup	defaxsu_;

	void			handleAutoScale();
	void			newDevSize();
	void			newColID();
    };

    uiDataPointSet&		uidps_;
    const DataPointSet&		dps_;
    Setup			setup_;
    MouseEventHandler&		meh_;
    AxisData			x_;
    AxisData			y_;
    AxisData			y2_;
    LinStats2D&			lsy1_;
    LinStats2D&			lsy2_;
    bool			doy2_;
    bool			dobd_;
    int				eachrow_;
    int				curgrp_;
    const DataPointSet::ColID	mincolid_;
    DataPointSet::RowID		selrow_;
    bool			selrowisy2_;

    void 			initDraw(CallBacker*);
    virtual void		mkNewFill();
    void 			drawContent(CallBacker*);
    void 			calcStats();

    int				getRow(const AxisData&,uiPoint) const;

    bool			selNearest(const MouseEvent&);
    void 			mouseClick(CallBacker*);
    void 			mouseRel(CallBacker*);

    void 			drawData(const AxisData&);

    friend class		uiDataPointSetCrossPlotWin;
    friend class		AxisData;
};


#endif

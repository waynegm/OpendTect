#ifndef horzvalueprovider_h
#define horzvalueprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Decemeber 2016
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "zvalueprovider.h"
#include "coord.h"
#include "color.h"
#include "ptrman.h"
#include "uistring.h"

class TrcKey;
class MultiID;
namespace EM { class Horizon; }

mExpClass(EarthModel) HorZValueProvider : public ZValueProvider
{
public:
				HorZValueProvider(const EM::Horizon*);
    virtual uiString		getName() const;
    virtual int			depthID() const		{ return depthid_; }
    void			setDepthID( int id )	{ depthid_ = id; }
    void			setHorizon(const EM::Horizon*);
    const EM::Horizon*		horizon() const { return hor_.ptr(); }

    virtual Color		drawColor() const;
    virtual int			drawWidth() const;
    virtual bool		isOK() const		{ return hor_; }
    virtual float		getZValue(const TrcKey&) const;
    virtual float		getZValue(const Coord&) const;
protected:
    ConstRefMan<EM::Horizon>	hor_;
    int				depthid_;
};
#endif

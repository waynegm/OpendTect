#ifndef zvalueprovider_h
#define zvalueprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Decemeber 2016
________________________________________________________________________


-*/

#include "generalmod.h"
#include "color.h"
#include "uistring.h"
#include "bufstringset.h"
#include "factory.h"

class TrcKey;
class TrcKeySampling;
class TaskRunner;

mExpClass(General) ZValueProvider
{
public:
    mDefineFactory3ParamInClass( ZValueProvider,const IOPar&,
				const TrcKeySampling&,TaskRunner*,factory);
    virtual			~ZValueProvider()		{}
    static const char*		sType() { return "ZVal Provider Type"; }
    virtual uiString		getName() const			=0;
    virtual int			depthID() const			=0;
				//ID to denote the depth of event
    virtual Color		drawColor() const		=0;
    virtual int			drawWidth() const		=0;
    virtual bool		isOK() const			=0;
    virtual float		getZValue(const TrcKey&) const	=0;
    virtual float		getZValue(const Coord&) const	=0;
    virtual void		fillPar(IOPar&) const		=0;
};

#endif

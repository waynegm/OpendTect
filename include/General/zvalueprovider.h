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

class TrcKey;
class MultiID;

mExpClass(General) ZValueProvider
{
public:
    virtual			~ZValueProvider();
    virtual uiString		getName() const			=0;
    virtual int			depthID() const			=0;
    virtual Color		drawColor() const		=0;
    virtual int			drawWidth() const		=0;
    virtual bool		isOK() const			=0;
    virtual float		getZValue(const TrcKey&) const	=0;
    virtual float		getZValue(const Coord&) const	=0;
};

#endif

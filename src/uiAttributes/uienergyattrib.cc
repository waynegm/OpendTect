/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:		$Id: uienergyattrib.cc,v 1.5 2006-04-03 13:30:32 cvshelene Exp $
________________________________________________________________________

-*/


#include "uienergyattrib.h"
#include "energyattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;


uiEnergyAttrib::uiEnergyAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, inpfld );
    setHAlignObj( gatefld );
}


bool uiEnergyAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Energy::attribName()) )
	return false;

    mIfGetFloatInterval( Energy::gateStr(), gate, gatefld->setValue(gate) );
    return true;
}


bool uiEnergyAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiEnergyAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Energy::attribName()) )
	return false;

    mSetFloatInterval( Energy::gateStr(), gatefld->getFInterval() );
    return true;
}


bool uiEnergyAttrib::getInput( Attrib::Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


void uiEnergyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, Energy::gateStr() );
}

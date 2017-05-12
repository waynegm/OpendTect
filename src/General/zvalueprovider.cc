/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Decemeber 2016
________________________________________________________________________


-*/

#include "zvalueprovider.h"
#include "iopar.h"
#include "trckeysampling.h"

mImplFactory3Param( ZValueProvider,const IOPar&,const TrcKeySampling&,
		    TaskRunner*,ZValueProvider::factory );

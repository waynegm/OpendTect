/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "multiinputmod.h"
#include "multiinputstep.h"

mDefODPluginEarlyLoad(MultiInput)
mDefODPluginInfo(MultiInput)
{
    static PluginInfo retpi = {
	"MultiInput (base)",
	"dGB Earth Sciences",
	"=od",
    	"Test multiple inputs" };
    return &retpi;
}


mDefODInitPlugin(MultiInput)
{
    VolProc::MultiInputStep::initClass();
    return 0;
}

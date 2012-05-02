/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/

static const char* rcsID mUnusedVar = "$Id: uimadattribspi.cc,v 1.5 2012-05-02 15:11:15 cvskris Exp $";

#include "uimadagcattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiMadagascarAttribs)
{
    static PluginInfo retpii = {
	"Madagascar Attributes",
	"dGB - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." };
    return &retpii;
}


mDefODInitPlugin(uiMadagascarAttribs)
{
    uiMadAGCAttrib::initClass();

    return 0; // All OK - no error messages
}

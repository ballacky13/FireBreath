/**********************************************************\
Original Author: Richard Bateman (taxilian)

Created:    Dec 3, 2009
License:    Dual license model; choose one of two:
            New BSD License
            http://www.opensource.org/licenses/bsd-license.php
            - or -
            GNU Lesser General Public License, version 2.1
            http://www.gnu.org/licenses/lgpl-2.1.html

Copyright 2009 PacketPass, Inc and the Firebreath development team
\**********************************************************/

#define _GNU_SOURCE
#include <dlfcn.h>

#include "NpapiTypes.h"
#include "PluginCore.h"
#include "X11/PluginWindowX11.h"
#include "X11/FactoryDefinitionsX11.h"

#include "X11/NpapiPluginX11.h"

using namespace FB::Npapi;

inline GdkNativeWindow getGdkWindow(void *in)
{
    return (char*)in - (char*)0;
}

namespace 
{
    std::string getPluginPath() 
    {
        ::Dl_info dlinfo; 
        if (::dladdr((void*)::NP_Initialize, &dlinfo) != 0) { 
            return dlinfo.dli_fname; 
        } else {
            return "";
        }
    }
}

NpapiPluginX11::NpapiPluginX11(FB::Npapi::NpapiBrowserHostPtr host) : NpapiPlugin(host), pluginWin(NULL)
{
    PluginCore::setPlatform("X11", "NPAPI");
    
    // Get the path to the shared lib
    
    static const std::string pluginPath = getPluginPath(); 
    setFSPath(pluginPath);
}

NpapiPluginX11::~NpapiPluginX11()
{
    delete pluginWin; pluginWin = NULL;
}

NPError NpapiPluginX11::GetValue(NPPVariable variable, void *value)
{
    switch (variable) {
    /* Tell browser we use XEmbed, this is required for Chrome on Unix */
    case NPPVpluginNeedsXEmbed:
        *((bool *)value) = true;
        return NPERR_NO_ERROR;
    default:
        return NpapiPlugin::GetValue(variable, value);
    }
}

NPError NpapiPluginX11::SetWindow(NPWindow* window)
{
    if (window != NULL && window->window != NULL) {
        /* Require XEmbed support from browser, see:
         * https://developer.mozilla.org/en/XEmbed_Extension_for_Mozilla_Plugins
         */
        int xembedSupported = 0;

        m_npHost->GetValue(NPNVSupportsXEmbedBool, &xembedSupported);
        if (!xembedSupported)
        {
            printf("XEmbed not supported\n");
            return NPERR_GENERIC_ERROR;
        }

        if (pluginWin != NULL && pluginWin->getWindow() != getGdkWindow(window->window)) {
            pluginMain->ClearWindow();
            delete pluginWin; pluginWin = NULL;
        }

        if (pluginWin == NULL) {
            GdkNativeWindow browserWindow;
            m_npHost->GetValue(NPNVnetscapeWindow, (void*)&browserWindow);

            pluginWin = _createPluginWindow(getGdkWindow(window->window));
            pluginWin->setBrowserWindow(browserWindow);
            pluginMain->SetWindow(pluginWin);
        }

        pluginWin->setWindowPosition(window->x, window->y, window->width, window->height);
        pluginWin->setWindowClipping(window->clipRect.top, window->clipRect.left,
                                      window->clipRect.bottom, window->clipRect.right);
    } else if (pluginWin != NULL) {
        // If the handle goes to NULL, our window is gone and we need to stop using it
        pluginMain->ClearWindow();
        delete pluginWin; pluginWin = NULL;
    }

    return NPERR_NO_ERROR;
}


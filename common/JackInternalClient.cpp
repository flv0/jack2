/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2006 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

#include "JackInternalClient.h"
#include "JackEngine.h"
#include "JackServer.h"
#include "JackGraphManager.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackInternalClientChannel.h"
#include <assert.h>

namespace Jack
{

JackGraphManager* JackInternalClient::fGraphManager = NULL;
JackEngineControl* JackInternalClient::fEngineControl = NULL;

// Used for external C API (JackAPI.cpp)
JackGraphManager* GetGraphManager()
{
    return JackServer::fInstance->GetGraphManager();
}

JackEngineControl* GetEngineControl()
{
    return JackServer::fInstance->GetEngineControl();
}

JackSynchro** GetSynchroTable()
{
    return JackServer::fInstance->GetSynchroTable();
}

JackInternalClient::JackInternalClient(JackServer* server, JackSynchro** table): JackClient(table)
{
    fClientControl = new JackClientControl();
    fChannel = new JackInternalClientChannel(server);
}

JackInternalClient::~JackInternalClient()
{
    delete fClientControl;
    delete fChannel;
}

int JackInternalClient::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
    int result;
	char name_res[JACK_CLIENT_NAME_SIZE]; 
    JackLog("JackInternalClient::Open name = %s\n", name);
	
	snprintf(fServerName, sizeof(fServerName), server_name);
 	
	fChannel->ClientCheck(name, name_res, JACK_PROTOCOL_VERSION, (int)options, (int*)status, &result);
    if (result < 0) {
		int status1 = *status;
        if (status1 & JackVersionError)
			jack_error("JACK protocol mismatch %d", JACK_PROTOCOL_VERSION);
        else
			jack_error("Client name = %s conflits with another running client", name);
        goto error;
    }
	
	strcpy(fClientControl->fName, name_res);

    // Require new client
    fChannel->ClientOpen(name_res, &fClientControl->fRefNum, &fEngineControl, &fGraphManager, this, &result);
    if (result < 0) {
        jack_error("Cannot open client name = %s", name_res);
        goto error;
    }

    SetupDriverSync(false);
    return 0;

error:
    fChannel->Stop();
    fChannel->Close();
    return -1;
}

JackGraphManager* JackInternalClient::GetGraphManager() const
{
    assert(fGraphManager);
    return fGraphManager;
}

JackEngineControl* JackInternalClient::GetEngineControl() const
{
    assert(fEngineControl);
    return fEngineControl;
}

JackClientControl* JackInternalClient::GetClientControl() const
{
    return fClientControl;
}

JackLoadableInternalClient::JackLoadableInternalClient(JackServer* server, JackSynchro** table, const char* so_name, const char* object_data)
	:JackInternalClient(server, table)
{
	char path_to_so[PATH_MAX + 1];
	//snprintf(path_to_so, sizeof(path_to_so), ADDON_DIR "/%s.so", so_name);
	snprintf(path_to_so, sizeof(path_to_so), so_name);
	snprintf(fObjectData, JACK_LOAD_INIT_LIMIT, object_data);
	fHandle = LoadJackModule(path_to_so);
	
	JackLog("JackLoadableInternalClient::JackLoadableInternalClient path_to_so = %s\n", path_to_so);
	
	if (fHandle == 0) {
		jack_error("error loading %s", so_name);
		throw -1;
	}

	fInitialize = (InitializeCallback)GetJackProc(fHandle, "jack_initialize");
	if (!fInitialize) {
		UnloadJackModule(fHandle);
		jack_error("symbol jack_initialize cannot be found in %s", so_name);
		throw -1;
	}

	fFinish = (FinishCallback)GetJackProc(fHandle, "jack_finish");
	if (!fFinish) {
		UnloadJackModule(fHandle);
		jack_error("symbol jack_finish cannot be found in %s", so_name);
		throw -1;
	}
}

JackLoadableInternalClient::~JackLoadableInternalClient()
{
	if (fFinish) 
		fFinish(fProcessArg);
	UnloadJackModule(fHandle);
}

int JackLoadableInternalClient::Open(const char* server_name, const char* name, jack_options_t options, jack_status_t* status)
{
	int res = JackInternalClient::Open(server_name, name, options, status);
	if (res == 0) 
		fInitialize((jack_client_t*)this, fObjectData);
	return res;
}

} // end of namespace


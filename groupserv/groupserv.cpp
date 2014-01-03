/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2008 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include "../common/debug.h"
#include "../common/opcodemgr.h"
#include "../common/EQStreamFactory.h"
#include "../common/rulesys.h"
#include "../common/servertalk.h"
#include "../common/platform.h"
#include "../common/crash.h"
#include "../common/EQEmuConfig.h"
#include "database.h"
#include "worldserver.h"
#include <list>
#include <signal.h>


TimeoutManager timeout_manager;
Database database;
volatile bool RunLoops = true;
WorldServer *worldserver = nullptr;

void CatchSignal(int sig_num) {

	RunLoops = false;

	if(worldserver)
		worldserver->Disconnect();
}

int main() {
	RegisterExecutablePlatform(ExePlatformGroupServ);
	set_exception_handler();

	Timer InterserverTimer(INTERSERVER_TIMER); // does auto-reconnect

	_log(GROUPSERV__INIT, "Starting EQEmu GroupServ.");

	if (!EQEmuConfig::LoadConfig()) {
		_log(GROUPSERV__INIT, "Loading server configuration failed.");
		return 1;
	}

	const EQEmuConfig *config = EQEmuConfig::get();

	if(!load_log_settings(config->LogSettingsFile.c_str()))
		_log(GROUPSERV__INIT, "Warning: Unable to read %s", config->LogSettingsFile.c_str());
	else
		_log(GROUPSERV__INIT, "Log settings loaded from %s", config->LogSettingsFile.c_str());

	_log(GROUPSERV__INIT, "Connecting to MySQL...");

	if (!database.Connect(
		config->DatabaseHost.c_str(),
		config->DatabaseUsername.c_str(),
		config->DatabasePassword.c_str(),
		config->DatabaseDB.c_str(),
		config->DatabasePort)) {
		_log(GROUPSERV__INIT_ERR, "Cannot continue without a database connection.");
		return 1;
	}

	if (signal(SIGINT, CatchSignal) == SIG_ERR)	{
		_log(GROUPSERV__ERROR, "Could not set signal handler");
		return 1;
	}
	if (signal(SIGTERM, CatchSignal) == SIG_ERR)	{
		_log(GROUPSERV__ERROR, "Could not set signal handler");
		return 1;
	}

	worldserver = new WorldServer(config->SharedKey);
	worldserver->Connect();

	while(RunLoops) {

		Timer::SetCurrentTime();

		if (InterserverTimer.Check()) {
			if (worldserver->TryReconnect() && (!worldserver->Connected()))
				worldserver->AsyncConnect();
		}
		worldserver->Process();

		timeout_manager.CheckTimeouts();

		Sleep(10);
	}
}

void UpdateWindowTitle(char* iNewTitle) {
#ifdef _WINDOWS
	char tmp[500];
	if (iNewTitle) {
		snprintf(tmp, sizeof(tmp), "GroupServ: %s", iNewTitle);
	}
	else {
		snprintf(tmp, sizeof(tmp), "GroupServ");
	}
	SetConsoleTitle(tmp);
#endif
}

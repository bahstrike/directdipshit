#pragma once

#include <Windows.h>
#include <comdef.h>
#include <initguid.h>//#define INITGUID  might only need this- havent tried it// must be included before creating GUIDs and maybe even before teh dplay crap
#include <stdio.h>
#include <stdarg.h>
#include <dplay.h>
#include <dplobby.h>
#include <vector>


// forward declarations
class READPACKET;



#include "jklowlvl.h"

///////////////////////////////////////////////////////////////////////////
//    WHEN PROGRAM IS RUNNING, HOLD   CTRL+ALT  FOR A SEC TO GRACEFULLY EXIT
///////////////////////////////////////////////////////////////////////////




#define MAXPLAYERS 32
#define SESSIONNAME "DIRECTDIPSHIT HERE"
#define PLAYERNAME "DIRECTDIPSHIT"



//   LocalHost:  TRUE     USE WHEN HOSTING A REAL JK GAME     set ipAddress for your local adapter  (best results: use real local IP rather than 127.0.0.1, if u can.  otherwise packets may be lost from other clients)
//														      in this mode, the checksum value will be scraped from JK memory for easy joining. (preferred mode for debugging)
//
//   LocalHost:  FALSE    USE WHEN JOINING REMOTE JK GAME     set ipAddress to a remote session.   you will connect but have to wait until another player joins in order to
//															  scrape the checksum value from their join message.
const bool s_bLocalHost = true;
const char s_ipAddress[] = "192.168.5.2";



// must end with a slash  (primitive preprocessor string cats)
#define LOGPATH "c:\\directdipshit\\"



// blacklist some IDs for dev purposes to prevent logfile flooding
const unsigned int s_uBlacklistedMsgID[] = {
	MSGID_ThingSync,
	MSGID_Dunno_SomeSync4,
	MSGID_Dunno_SomeSync5,
	MSGID_Dunno_SomeSync6,
	MSGID_ServerInfo_PlayerList,
};


// whether u want a lot of the repeating sync messages in the logfile
const bool s_bIgnoreSyncForLog = true;



// our common stuff after constants
#include "util.h"
#include "log.h"
#include "packet.h"
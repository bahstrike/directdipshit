#pragma once

#include <Windows.h>
#include <comdef.h>
#include <initguid.h>//#define INITGUID  might only need this- havent tried it// must be included before creating GUIDs and maybe even before teh dplay crap
#include <stdio.h>
#include <stdarg.h>
#include <dplay.h>
#include <dplobby.h>
#include <vector>




#define MAXPLAYERS 32
#define SESSIONNAME "DIRECTDIPSHIT HERE"
#define PLAYERNAME "DIRECTDIPSHIT"

// must end with a slash
#define LOGPATH "c:\\directdipshit\\"


#define IGNORESYNCMSG 1


// WARNING FOR LOCAL HOST:  if debugging multi-user sessions, cannot use 127.0.0.1  as we will not be sent other players' packets.  using the actual local adapter fixes this
const bool s_bLocalHost = true;
const char s_ipAddress[] = "192.168.5.2";//"192.168.5.3";//"127.0.0.1";



// our common stuff after constants
#include "log.h"
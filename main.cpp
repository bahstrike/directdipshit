#include "dipshit.h"
#include "haxx.h"

IDirectPlay3 *g_pDirectPlay = nullptr;

DPSESSIONDESC2* g_pSession = nullptr;

char* g_szJKLName = nullptr;


BOOL FAR PASCAL DPlayEnumSessions(
	LPCDPSESSIONDESC2 lpSessionDesc, LPDWORD lpdwTimeOut,
	DWORD dwFlags, LPVOID lpContext)
{
	HWND   hWnd = (HWND)lpContext;
	//LPGUID lpGuid;
	//LONG   iIndex;

	// Determine if the enumeration has timed out.
	if (dwFlags & DPESC_TIMEDOUT)
		return (FALSE);            // Do not try again

	int sessionNameLen = wcslen(lpSessionDesc->lpszSessionName);
	char* szSessionName = new char[sessionNameLen + 1];
	wcstombs(szSessionName, lpSessionDesc->lpszSessionName, sessionNameLen);
	szSessionName[sessionNameLen] = 0;

	Log("FOUND SESSION \"%s\"  players %d/%d", szSessionName, lpSessionDesc->dwCurrentPlayers, lpSessionDesc->dwMaxPlayers);


	// extract and keep JKL name for later use in joinrequest
	for (int i = sessionNameLen - 1; i >= 0; i--)
	{
		char c = szSessionName[i];
		if (c == ':')
		{
			int jklLen = sessionNameLen - i;
			g_szJKLName = new char[jklLen];
			strcpy(g_szJKLName, szSessionName + i + 1);

			//Log("Stored JKL name %s  for later use in joinrequest", g_szJKLName);

			break;
		}
	}


	delete[] szSessionName;

	if (g_pSession == nullptr)
	{
		// save session copy
		g_pSession = new DPSESSIONDESC2;
		memcpy(g_pSession, lpSessionDesc, sizeof(DPSESSIONDESC2));
	}

	return (TRUE);
}


/*BOOL FAR PASCAL DPlayEnumGroups(
	DPID            dpId,
	DWORD           dwPlayerType,
	LPCDPNAME       lpName,
	DWORD           dwFlags,
	LPVOID          lpContext)
{

	return (TRUE);
}*/

DPNAME* g_pPlayerTemplate = nullptr;

BOOL FAR PASCAL DPlayEnumPlayers(
	DPID            dpId,
	DWORD           dwPlayerType,
	LPCDPNAME       lpName,
	DWORD           dwFlags,
	LPVOID          lpContext)
{
	int playerNameLen = wcslen(lpName->lpszShortName);
	char* szPlayerName = new char[playerNameLen + 1];
	wcstombs(szPlayerName, lpName->lpszShortName, playerNameLen);
	szPlayerName[playerNameLen] = 0;

	Log("FOUND PLAYER \"%s\"   DPID: %d", szPlayerName, dpId);

	// steal this shit so we have a compatible player struct
	if (g_pPlayerTemplate == nullptr)
	{
		g_pPlayerTemplate = new DPNAME;
		memcpy(g_pPlayerTemplate, lpName, sizeof(DPNAME));
	}

	return (TRUE);
}


void CreateSession()
{
	HRESULT hr;

	DPSESSIONDESC2 sessionDesc;
	ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
	sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
	sessionDesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
	sessionDesc.guidApplication = *GimmeJKGUID();
	CoCreateGuid(&sessionDesc.guidInstance);
	sessionDesc.dwMaxPlayers = MAXPLAYERS;
	sessionDesc.lpszSessionNameA = const_cast<char*>(SESSIONNAME);


	hr = g_pDirectPlay->Open(&sessionDesc, DPOPEN_CREATE);
	Log("%s = DirectPlay->Open(CREATE)", FormatDPLAYRESULT(hr));
	if (hr != S_OK)
		return;



	Log("Closing Session");
	g_pDirectPlay->Close();
}

unsigned int g_uChecksum = 0;
bool g_bAttemptedJoin = false;
unsigned short g_nINGAMEJOINRequestPacketNum = 0;
unsigned int g_uInGameJoinPlayerToken = 0;
DPID g_hostDpid = 0;
DPID g_localPlayer = 0;// this be us

DWORD g_uActuallyInTimestamp = 0;




int g_nMaxPlayerSlots = 0;
PLAYERSLOT* g_pPlayerSlots = nullptr;
int g_nLocalPlayerIndex = -1;

int FindPlayerSlotIndexByDPID(DPID dpid, bool mustBeConnected=true)
{
	if (g_pPlayerSlots == nullptr)
		return -1;

	for (int x = 0; x < g_nMaxPlayerSlots; x++)
	{
		PLAYERSLOT& slot = g_pPlayerSlots[x];

		if (!slot.hasData())
			continue;

		if (slot.dpid == dpid && (!mustBeConnected || slot.isConnected()))
			return x;
	}

	return -1;
}

PLAYERSLOT* FindPlayerSlotByDPID(DPID dpid, bool mustBeConnected = true)
{
	int index = FindPlayerSlotIndexByDPID(dpid, mustBeConnected);
	if (index == -1)
		return nullptr;

	return &g_pPlayerSlots[index];
}


void PacketNumAck(DPID senderDpid, unsigned short packetNum)
{
	WRITEPACKET p;
	p.writeUInt16(MSGID_Dunno_PacketFlush);
	p.writeUInt16(0);
	p.writeUInt16(packetNum);

	p.send(senderDpid);


	Log("SEND PACKETNUM ACK: %d", packetNum);
}

void ProcessPacket(DPID senderDpid, READPACKET& p, int dumpIndex)
{
	MESSAGEID msgID = (MESSAGEID)p.readUInt16();
	unsigned short packetNum = p.readUInt16();

	// blacklist some IDs for dev purposes to prevent flooding
	bool blacklist = false;
	for (int x = 0; !blacklist && x < sizeof(s_uBlacklistedMsgID) / sizeof(*s_uBlacklistedMsgID); x++)
		if (s_uBlacklistedMsgID[x] == msgID)
			blacklist = true;


	bool logPacket = !s_bIgnoreSyncForLog || !blacklist;

	if(logPacket)
		Log("ProcessPacket (dump:%d,  len:%d) senderDpid:%d  msgID:0x%02X  packetNum:%d  msg:%s", dumpIndex, p.getTotalLength(), senderDpid, msgID, packetNum, MessageIDToString(msgID));

	if (msgID == MSGID_HostStatus)
	{
		// for 118 bytes

		int d1 = p.readInt32();
		int d2 = p.readInt32();
		unsigned int d3 = p.readUInt32();
		unsigned int d4 = p.readUInt32();

		float f1 = p.readFloat();
		float f2 = p.readFloat();
		float f3 = p.readFloat();

		char sab0[32];
		p.read(sab0, 32);

		char sab1[32];
		p.read(sab1, 32);

		int d5 = p.readInt32();
		int d6 = p.readInt32();
		int d7 = p.readInt32();
		int d8 = p.readInt32();
		int d9 = p.readInt32();
		unsigned short d10 = p.readInt16();

		Log("HOSTSTATUSLOLL   d1:%d    d2:%d    d3:0x%08X    d4:0x%08X    f1:%0.2f   f2:%0.2f   f3:%0.2f    sab0:%s    sab1:%s    d5:%d   d6:%d   d7:%d   d8:%d   d9:%d   d10:%d",
			d1, d2, d3, d4, f1, f2, f3, sab0, sab1, d5, d6, d7, d8, d9, d10);


	} else if (msgID == DPSYS_CREATEPLAYERORGROUP)
	{
		DWORD playerType = p.readUInt32();//might not have a full struct for 0 type

		DPMSG_CREATEPLAYERORGROUP createPlayer;
		createPlayer.dwType = msgID;
		createPlayer.dwPlayerType = playerType;

		if (playerType == DPPLAYERTYPE_PLAYER)
		{
			p.read((char*)&createPlayer + 8/*we already read the message ID and player type*/, sizeof(DPMSG_CREATEPLAYERORGROUP) - 8);

			Log("PLAYERCREATE plrName:%s", UnsafeTmpMBS(createPlayer.dpnName.lpszShortName));
		}
		else if (playerType == 0/*DPPLAYERTYPE_GROUP   i dunno if JK supports groups-  they might be usin some shorthand shit here for 0*/)
		{
			unsigned short dunno = p.readUInt16();
			Log("PLAYERCREATE  SHORTHAND   dunno:%d", dunno);
		}

	} else 	if (msgID == DPSYS_DESTROYPLAYERORGROUP)
	{
		DWORD playerType = p.readUInt32();//might not have a full struct for 0 type

		DPMSG_DESTROYPLAYERORGROUP destroyPlayer;
		destroyPlayer.dwType = msgID;
		destroyPlayer.dwPlayerType = playerType;

		// DIDNT ACTUALLY TEST IF THEY USE SHORTHAND FOR DESTROY  DURING CUSTOM LEVELS,  BUT I MEAN... PROBABLY..  MIGHT AS WELL ATTEMPT TO SUPPORT (not like we usin the data anyway)

		if (playerType == DPPLAYERTYPE_PLAYER)
		{
			p.read((char*)&destroyPlayer + 8/*we already read the message ID and player type*/, sizeof(DPMSG_DESTROYPLAYERORGROUP) - 8);

			Log("PLAYERDESTROY plrName:%s", UnsafeTmpMBS(destroyPlayer.dpnName.lpszShortName));
		} else if (playerType == 0/*DPPLAYERTYPE_GROUP   i dunno if JK supports groups-  they might be usin some shorthand shit here for 0*/)
		{
			unsigned short dunno = p.readUInt16();
			Log("PLAYERDESTROY  SHORTHAND   dunno:%d", dunno);
		}
	} else if (msgID == MSGID_ServerInfo_PlayerList)
	{
		int dunno2 = p.readInt32();
		g_hostDpid = (DPID)p.readUInt32();
		int maxPlayerEntries = p.readInt16();

		//Log("Server info:  host DPID:%d   maxPlayers:%d", g_hostDpid, maxPlayerEntries);


		if (g_pPlayerSlots != nullptr)
		{
			delete[] g_pPlayerSlots;
			g_pPlayerSlots = nullptr;
		}
		g_nMaxPlayerSlots = 0;


		if (maxPlayerEntries > 0)
		{
			g_nMaxPlayerSlots = maxPlayerEntries;
			g_pPlayerSlots = new PLAYERSLOT[g_nMaxPlayerSlots];

			for (int x = 0; x < g_nMaxPlayerSlots; x++)
				g_pPlayerSlots[x].read(p);
		}

		g_uActuallyInTimestamp = GetTickCount();
	}
	else if (msgID == MSGID_JoinRequestResponse)
	{
		JOINRESULT jr = (JOINRESULT)p.readUInt32();
		short dunno1 = p.readInt16();
		short dunno2 = p.readInt16();

		// if host was busy from our attempt, clear flag to try again
		if (jr == JR_BUSY)
			g_bAttemptedJoin = false;

		Log("JOIN RESULT (%d): %s   d1:%d    d2:%d", jr, JoinResultToString(jr), dunno1, dunno2);
	}
	else if (msgID == MSGID_JoinRequest)
	{
		JOINREQUEST pjoin;
		p.read(&pjoin, sizeof(JOINREQUEST));

		if (g_uChecksum == 0)
		{
			Log("STEALING CHECKSUM FROM %s's JOINREQUEST: %d", UnsafeTmpMBS(pjoin.plrName), pjoin.checksum);
			g_uChecksum = pjoin.checksum;
		}

		msgID = msgID;
	}
	else if (msgID == MSGID_Dunno_InGameJoin)
	{
		int playerSlotIndex = p.readInt32();
		int joinToken = p.readInt32();	// not sure what this is, but seems to actually be in the directplay header itself.  might be DPID but doesnt look like (unless its byteswapped)

		if (p.getTotalLength() > 44)
		{
			Log("ERRMM OOOOOOPS 0x21  CAN ACTUALLY BE LONGER");
		}

		// player name string in unicode  but maybe some other stuff
		wchar_t wcs[16];
		p.read(wcs, sizeof(wcs));

		Log("DUNNO PLRJOIN %d:  %s   token:0x%08X", playerSlotIndex, UnsafeTmpMBS(wcs), joinToken);

		// hopefully this is OUR message
		Log("HOPEFULLY ITS FOR US- STEALING VALUES");
		g_nLocalPlayerIndex = playerSlotIndex;

		g_uInGameJoinPlayerToken = joinToken;

		g_nINGAMEJOINRequestPacketNum = packetNum;

		g_uActuallyInTimestamp = GetTickCount();
	}
	else if (msgID == MSGID_Dunno_PacketFlush)
	{
		unsigned short umKnownPacketNum = p.readUInt16();

		// uhh sure buddy.. we got all ur messages   (i mean actually in the future we should have a queue but whatev)

		
		// send back an acknowledgement-  this should cause packetNum to reset to 0  and probably keep us in the game
		PacketNumAck(senderDpid, umKnownPacketNum);
	}
	else if (msgID == MSGID_Maybe_ConsoleMessage)
	{
		int plrIndexDest = p.readInt32();// probably for team chat
		int plrIndexSrc = p.readInt32();// source?

		int msgLen = p.readInt32();// pretty sure
		char* sz = new char[msgLen+1];
		p.read(sz, msgLen);
		sz[msgLen] = 0;


		PLAYERSLOT* pSlot = nullptr;
		if (plrIndexSrc >= 0 && plrIndexSrc < g_nMaxPlayerSlots)
		{
			pSlot = &g_pPlayerSlots[plrIndexSrc];
			if (!pSlot->hasData() || !pSlot->isConnected())
				pSlot = nullptr;
		}

		Log("CONSOLE MESSAGE!!:  %s says, '%s'", (pSlot == nullptr ? "<UNKNOWN>" : pSlot->name), sz);


		delete[] sz;


		// send back ack for this puppy
		PacketNumAck(senderDpid, packetNum);
	}
	else if (msgID == MSGID_PlayerInfo1 || msgID == MSGID_PlayerInfo2)
	{
		int slotIndex = p.readInt32();

		DUNNOPLAYERINFO plr;
		p.read(&plr, sizeof(plr));


		//Log("SOME PLAYERINFO!!  playerSlot:%d    model:%s   sndcls:%s    saber0:%s   saber1:%s", slotIndex, plr.model, plr.soundclass, plr.saber0, plr.saber1);

		// might need to ack it
		PacketNumAck(senderDpid, packetNum);
	}
	else if (msgID == MSGID_ThingSync)
	{
		char msg[512];
		msg[0] = 0;

		int thingIndex = p.readInt32();
		unsigned short attachFlags = p.readUInt16();
		unsigned short maybeThingSig = p.readUInt16();

		float px = p.readFloat();
		float py = p.readFloat();
		float pz = p.readFloat();
		
		float pitch = p.readFloat();
		float yaw = p.readFloat();
		float roll = p.readFloat();

		sprintf(msg + strlen(msg), "THINGSYNC (%d): attachFlags:0x%04X  thingsig?:%d  POS:%0.2f/%0.2f/%0.2f  PYR:%0.2f/%0.2f/%0.2f  ",
			thingIndex, attachFlags, maybeThingSig, px, py, pz, pitch, yaw, roll);

		//p.getTotalLength()

		// THIS IS VARIABLE-LENGTH PACKET.   we might not have following values.
		// we are probably supposed to know how many values to accept by looking at Thing values-  but directdipshit does
		// not have an actual world loaded..  so we will just haxx it based upon packet length

		bool isMoveTypePhysics = (p.getTotalLength() > 36);

		if (isMoveTypePhysics)
		{
			unsigned int physicsFlags = p.readUInt32();

			float vx = p.readFloat();
			float vy = p.readFloat();
			float vz = p.readFloat();

			sprintf(msg + strlen(msg), " physicsflags:0x%08X  VEL:%0.2f/%0.2f/%0.2f  ", physicsFlags, vx, vy, vz);


			bool hasAngVel = (p.getTotalLength() >= 64);

			if (hasAngVel)
			{
				float pitchVel = p.readFloat();
				float yawVel = p.readFloat();
				float rollVel = p.readFloat();

				sprintf(msg + strlen(msg), "  ANGVEL:%0.2f/%0.2f/%0.2f  ", pitchVel, yawVel, rollVel);
			}
			else {
				// if we dont have ang vel, we still seem to have some extra value here.
				//  for players, this appears to be the aim pitch

				float aimPitch = p.readFloat();

				sprintf(msg + strlen(msg), "  aimPitch:%0.2f ", aimPitch);
			}
		}

		if (!s_bIgnoreSyncForLog)
			Log(msg);

		msgID = msgID;
	}
	else if (msgID == MSGID_Dunno_SomeSync4)
	{
		// prolly another variable-length
#if false
		int thingIndex = p.readInt32();
		int dunno1 = p.readInt32();
		int dunno2 = p.readInt32();
		unsigned int dunno3 = p.readUInt32();

		float f1 = p.readFloat();
		float f2 = p.readFloat();
		float f3 = p.readFloat();

		unsigned int dunno4 = p.readUInt32();
		int dunno5 = p.readInt32();
		int dunno6 = p.readInt32();
		float f4 = p.readFloat();
		int dunno7 = p.readInt32();
		float f5 = p.readFloat();

		if(!ignoreSyncMsg)
			Log("DUNNOSYNC (%d):  d1:%d  d2:%d  d3:0x%08X   POS:%0.2f/%0.2f/%0.2f   d4:0x%08X   d5:%d   d6:%d    f4:%f   d7:%d   f5:%f",
				thingIndex, dunno1, dunno2, dunno3, f1, f2, f3, dunno4, dunno5, f4, dunno7, f5);
#endif
	}
	else if (msgID == MSGID_Dunno_SomeSync5)
	{
		unsigned short dunno1 = p.readUInt16();
		int dunno2 = p.readInt32();

		if (!s_bIgnoreSyncForLog)
			Log("DUNSYNC5   d1:0x%04X   d2:%d", dunno1, dunno2);
	}
	else if (msgID == MSGID_Dunno_SomeSync6)
	{
		unsigned short dunno1 = p.readUInt16();
		int dunno2 = p.readInt32();

		float f1maybe = p.readFloat();

		if (!s_bIgnoreSyncForLog)
			Log("DERPSONC6   d1:0x%04X   d2:%d   f1maybe:%0.02f", dunno1, dunno2, f1maybe);
	}
	else if (msgID == MSGID_Dunno_SomeSync1)
	{
		unsigned short dunno1 = p.readUInt16();
		unsigned short dunno2 = p.readUInt16();
		unsigned short dunno3 = p.readUInt16();
		unsigned short dunno4 = p.readUInt16();

		float f1 = p.readFloat();
		float f2 = p.readFloat();
		float f3 = p.readFloat();
		float f4 = p.readFloat();
		float f5 = p.readFloat();
		float f6 = p.readFloat();
		float f7 = p.readFloat();
		float f8 = p.readFloat();
		float f9 = p.readFloat();
		float f10 = p.readFloat();
		
		
		if (!s_bIgnoreSyncForLog)
			Log("DOINK1   d1:0x%04X  d2:0x%04X  d3:0x%04X  d4:0x%04X  f1:%0.02f  f2:%0.02f  f3:%0.02f  f4:%0.02f  f5:%0.02f  f6:%0.02f  f7:%0.02f  f8:%0.02f  f9:%0.02f  f10:%0.02f  ",
				dunno1, dunno2, dunno3, dunno4, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
	}


	int remainBytes = p.getTotalLength() - p.getCurrentPosition();
	if (remainBytes > 0)
	{
		if(logPacket)
			Log("DIDNT READ EVERYTHING.. %d remain", remainBytes);
	}
}

void EmulatePacketFromFile(const char* szDumpFile)
{
	FILE* f = fopen(szDumpFile, "rb");
	fseek(f, 0, SEEK_END);
	int msgLen = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* msg = new char[msgLen];
	fread(msg, 1, msgLen, f);
	fclose(f);

	READPACKET p(msg, msgLen);

	ProcessPacket(0/*if we need DPIDs during emulation then we will do it later*/, p, -1);

	delete[] msg;
}

void ReadPackets()
{
	// gotcha bitch- lets get ur data
	HRESULT hr;

	DPID remotePlayer = 0;// host?
	DWORD msgSize = 0;
	hr = g_pDirectPlay->Receive(&remotePlayer, &g_localPlayer, DPRECEIVE_ALL, nullptr, &msgSize);
	if (hr == DPERR_NOMESSAGES)
		return;

	if (msgSize <= 0)
	{
		Log("PLR EVENT RECEIVE WTF MSG 0 LEN  SUX DIX");
		return;
	}

	//Log("PLAYER EVENT-  GOTTA Receive()  and see what it wants");

	char* msg = new char[msgSize];
	hr = g_pDirectPlay->Receive(&remotePlayer, &g_localPlayer, DPRECEIVE_ALL, msg, &msgSize);

	static int numreceived = 0;
	char dumpfile[MAX_PATH];
	sprintf(dumpfile, GenerateLogFilePath("dplayRECV%d.dmp"), numreceived);
	FILE* f = fopen(dumpfile, "wb");
	fwrite(msg, 1, msgSize, f);
	fclose(f);


	READPACKET p(msg, msgSize);
	ProcessPacket(remotePlayer, p, numreceived);

	//Log("DUMPED NET PACKET TO: %s", dumpfile);

	numreceived++;
	delete[] msg;

}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	LogInit();

	HRESULT hr;


	//MessageBox(0, "DirectDumbass IS TRYING TO HAVE FUN", 0, 0);
	Log("----------------------------------------------");
	Log("%s IS TRYING TO HAVE FUN", SESSIONNAME);
	Log("HOLD  CTRL+ALT  FOR A MOMENT TO EXIT GRACEFULLY");
	Log("----------------------------------------------");

	CleanupOldDumpFiles();


	// EMULATE RECEIVING A PACKET BY JUST LOADING A DUMP FILE AND CALLING PROCESSPACKET
#if false
	EmulatePacketFromFile(LOGPATH"0x39 PLAYERINFO.dmp");

	goto exitmain;
#endif


	// IF HOSTING LOCALLY, TRY TO MEMHAX THE CHECKSUM OUT
	if (s_bLocalHost)
	{
		g_uChecksum = ExtractLocalhostChecksum();
		if (g_uChecksum != 0)
		{
			Log("I JUST STOLE THE CHECKSOME FROM JK MEMORY: 0x%08X", g_uChecksum);
		}
	}


	hr = CoInitialize(NULL);


	// gimme a directplay interface
	hr = CoCreateInstance(CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay3, (LPVOID*)&g_pDirectPlay);
	//Log("%s = CoCreateInstance(DirectPlay3)", FormatHRESULT(hr));
	if (hr != S_OK || g_pDirectPlay == nullptr)
	{
		hr = hr;
		return 0;
	}
	


#if false
	Log("Testing CreateSession...");
	CreateSession();
#endif


#if true
	Log("ENUMSESSIONS BEGIN");

	// establish connection via direct IP-  prevents popup of dplay UI asking for IP
	{
		IDirectPlayLobby2* pLobby = nullptr;
		hr = CoCreateInstance(CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayLobby2, (LPVOID*)&pLobby);
		if (hr != S_OK || pLobby == nullptr)
		{
			hr = hr;
			return 0;
		}

		char* addressConnection = nullptr;
		DWORD addressConnectionLen = 0;
		pLobby->CreateAddress(DPSPGUID_TCPIP, DPAID_INet, s_ipAddress, strlen(s_ipAddress) + 1, addressConnection, &addressConnectionLen);//query size

		addressConnection = new char[addressConnectionLen];
		hr = pLobby->CreateAddress(DPSPGUID_TCPIP, DPAID_INet, s_ipAddress, strlen(s_ipAddress) + 1, addressConnection, &addressConnectionLen);
		//Log("%s = DirectPlayLobby->CreateAddress", FormatDPLAYRESULT(hr));


		pLobby->Release();
		pLobby = nullptr;


		hr = g_pDirectPlay->InitializeConnection(addressConnection, 0);// seems like it can only be done once-  might have to release / recreate directplay object to connect elsewhere
		//Log("%s = DirectPlay->InitializeConnection", FormatDPLAYRESULT(hr));

		delete[] addressConnection;// assuming that initializeconnection copies internally so we can free
	}


	DPSESSIONDESC2 sessionDesc;
	ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
	sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
	sessionDesc.guidApplication = *GimmeJKGUID();
	hr = g_pDirectPlay->EnumSessions(&sessionDesc, 0, DPlayEnumSessions, NULL/*hWnd*/, DPENUMSESSIONS_AVAILABLE);
	//Log("%s = DirectPlay->EnumSessions", FormatDPLAYRESULT(hr));
	Log("ENUMSESSIONS END");
#endif



#if true
	Log("SESSION DETAILS BEGIN");
	if (g_pSession != nullptr)
	{
		hr = g_pDirectPlay->Open(g_pSession, DPOPEN_JOIN);
		//Log("%s = DirectPlay->Open(DPOPEN_JOIN)", FormatDPLAYRESULT(hr));

		// JK doesnt seem to use groups..
		/*hr = g_pDirectPlay->EnumGroups(NULL, DPlayEnumGroups, NULL, 0);
		Log("%s = DirectPlay->EnumSessions", FormatDPLAYRESULT(hr));*/

		hr = g_pDirectPlay->EnumPlayers(NULL, DPlayEnumPlayers, NULL, 0);
		//Log("%s = DirectPlay->EnumPlayers", FormatDPLAYRESULT(hr));


		// copy crap from enumplayers
		DPNAME localName;
		ZeroMemory(&localName, sizeof(DPNAME));
		localName.dwSize = sizeof(DPNAME);

		if (g_pPlayerTemplate != nullptr)
			memcpy(&localName, g_pPlayerTemplate, sizeof(DPNAME));

		// haxx name
		int slen = strlen(PLAYERNAME);
		wchar_t* wcs = new wchar_t[slen+1];
		mbstowcs(wcs, PLAYERNAME, slen);
		wcs[slen] = 0;

		localName.lpszShortName = wcs;

		hr = g_pDirectPlay->CreatePlayer(&g_localPlayer, &localName, NULL, NULL, 0, 0/*joining-  nonserver and nonspectator*/);
		//Log("%s = DirectPlay->CreatePlayer", FormatDPLAYRESULT(hr));


		delete[] wcs;



		Log("IN SESSION.. WAITING FOREVER UNTIL U HOLD CTRL+ALT");
		for (;;)
		{
			ReadPackets();


			// wait until we have a JKL name and a checksum, then we can try to join!!
			if (!g_bAttemptedJoin && g_szJKLName != nullptr && g_uChecksum != 0)
			{
				g_bAttemptedJoin = true;

				Log("WE HAVE DATA TO JOIN-  ATTEMPTING");


				JOINREQUEST jr;
				ZeroMemory(&jr, sizeof(JOINREQUEST));
				strcpy(jr.jklName, g_szJKLName);

				mbstowcs(jr.plrName, PLAYERNAME, sizeof(PLAYERNAME));

				jr.checksum = g_uChecksum;



				WRITEPACKET p;
				p.writeUInt16(MSGID_JoinRequest);
				p.writeUInt16(0);// no packetNum for this one it seems
				p.write(&jr, sizeof(JOINREQUEST));


				p.send(g_hostDpid, g_localPlayer, true);// dunno if needs to be gauranteed
			}

			
			// i think anytime we receive 0x21 we are supposed to send one back
			if (g_nINGAMEJOINRequestPacketNum != 0  && g_uInGameJoinPlayerToken != 0)
			{
				Log("SEND INGAMEJOIN RESPONSE");

				WRITEPACKET p;

				p.writeUInt16(MSGID_Dunno_InGameJoin);
				p.writeUInt16(g_nINGAMEJOINRequestPacketNum);


				// should convert to struct

				p.writeUInt32(g_nLocalPlayerIndex);
				p.writeUInt32(g_uInGameJoinPlayerToken);

				p.writeFixedWideString_MBS(PLAYERNAME, 16);


				p.send(g_hostDpid, g_localPlayer, true);


				// do we send our player info now?
				{
					Log("SENDING PLAYER INFO AFSLKASFHJAF");

					WRITEPACKET outPacket;
					outPacket.writeUInt16(MSGID_PlayerInfo1);
					outPacket.writeUInt16(g_nINGAMEJOINRequestPacketNum+1/*try issuing as next*/);

					outPacket.writeInt32(g_nLocalPlayerIndex);
					
					outPacket.writeFixedString("hepk.3do", 32);
					outPacket.writeFixedString("kybobafett.snd", 32);
					outPacket.writeFixedString("dark.mat", 32);
					outPacket.writeFixedString("dark.mat", 32);

					outPacket.send(g_hostDpid);
				}


				g_nINGAMEJOINRequestPacketNum = 0;
			}



			// BAD HAXX-  we should actually keep the timestamp we joined in at, but for now im
			// usin it for a timed-based flag to send a fuckin hidy ho message to everyone
			if (g_uActuallyInTimestamp != 0 && (GetTickCount() - g_uActuallyInTimestamp) >= 600)
			{
				g_uActuallyInTimestamp = 0;// we already did this now


				// construct a console message broadcast lol
				//Log("ADDING WARM WELCOME TO CHAT");



				WRITEPACKET p;

				p.writeUInt16(MSGID_Maybe_ConsoleMessage);
				p.writeUInt16(0);// dunno-  probably am supposed to fill in a valid packetNum !!!!!


				p.writeInt32(-1);// to?  (player index?  -1 is all)
				p.writeInt32(g_nLocalPlayerIndex);// from?  (player index?)

				const char* sz = "HIDY HO NEIGHBORS";
				p.writeInt32(strlen(sz)+1);
				p.write(sz, strlen(sz)+1);// a little confused-  i dont remember seeing in a null in wireshark packet but the last character gets cut off otherwise.. whatever


				p.send(g_hostDpid);
			}




			if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 && (GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
				break;
			Sleep(0);
		}
	}
	Log("SESSION DETAILS END");
#endif



	Log("CLEANUP");

	if (g_pPlayerSlots != nullptr)
	{
		delete[] g_pPlayerSlots;
		g_pPlayerSlots = nullptr;
	}
	g_nMaxPlayerSlots = 0;
	g_nLocalPlayerIndex = -1;

	if (g_szJKLName != nullptr)
	{
		delete[] g_szJKLName;
		g_szJKLName = nullptr;
	}

	if (g_localPlayer != 0)
	{
		g_pDirectPlay->DestroyPlayer(g_localPlayer);
		g_localPlayer = 0;
	}

	if (g_pPlayerTemplate != nullptr)
	{
		delete g_pPlayerTemplate;
		g_pPlayerTemplate = nullptr;
	}

	if (g_pSession != nullptr)
	{
		delete g_pSession;
		g_pSession = nullptr;
	}

	if (g_pDirectPlay != nullptr)
	{
		g_pDirectPlay->Release();
		g_pDirectPlay = nullptr;
	}

	CoUninitialize();

//exitmain:
	Log("----------------------------------------------");
	Log("EXITING");
	Log("----------------------------------------------");

	LogShutdown();
	return 0;
}

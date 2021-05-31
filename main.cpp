#include <Windows.h>
#include <comdef.h>
#include <initguid.h>//#define INITGUID  might only need this- havent tried it// must be included before creating GUIDs and maybe even before teh dplay crap
#include <stdio.h>
#include <stdarg.h>
#include <dplay.h>
#include <dplobby.h>
#include <vector>

//#pragma comment(lib, "cmcfg32.lib")// need for priv stuff?  maybe not

#define MAXPLAYERS 32
#define SESSIONNAME "DIRECTDIPSHIT HERE"
#define PLAYERNAME "DIRECTDIPSHIT"

#define LOGPATH "c:\\directdipshit\\"

/*DEFINE_GUID(JK_GUID, 
	0x5bfdb060, 0x6a4, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e//DPCHAT_GUID   TESTING FOR NOW... NEED JK KEY
	//0xbfF0613c0, 0x11d0de79, 0x0a000c999, 0x4BAD7624 //0BF0613C0 11D0DE79 0A000C999 4BAD7624//JK_GUID
);*/



/*
	CHECKSUM INFORMATION FROM SHINY:

	input salt appears to come from DPSESSIONDESC2::dwUser1


	his ideas for extracting levelchecksum and regenerating it  (salt-independent) are as follows:

so you could do like, known_salt ^ precalc_hash_0
and get the precalc_hash_0 by taking the checksum and xoring it with the bitwise inverse of the salt
I think
yeah so hash_N ^ salt_N ^ 0xFFFFFFFF = precalc_hash_0
then in the future you can just precalc_hash_0 ^ new_salt_N = current_hash

*/



GUID* GimmeJKGUID()
{
	static int _jkguid[] = {
		0x0BF0613C0,
		0x011D0DE79,
		0x0A000C999,
		0x04BAD7624
	};

	return (GUID*)_jkguid;
}


bool EnableDebugPrivilege()
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return   FALSE;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
	{
		CloseHandle(hToken);
		return false;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		CloseHandle(hToken);
		return false;
	}
	return true;
}



// TO BE USED WHEN HOSTING JK LOCALLY-  just get the checksum value from the process directly.
// IF THIS SHIT FAILS, YOU NEED TO RUN VEGETABLESTUDIO AS ADMIN
unsigned int ExtractLocalhostChecksum()
{
	static bool escalated = false;
	if (!escalated)
	{
		escalated = true;
		if (!EnableDebugPrivilege())
			return 0;
	}

	HWND hJKWnd = FindWindow("wKernel", "Jedi Knight");
	if (hJKWnd == NULL)
		return 0;

	DWORD jkProcessID;
	GetWindowThreadProcessId(hJKWnd, &jkProcessID);

	HANDLE hJK = OpenProcess(PROCESS_ALL_ACCESS, FALSE, jkProcessID);
	if (hJK == NULL)
		return 0;

	unsigned int checksum = 0;
	ReadProcessMemory(hJK, (void*)0x00832678, &checksum, 4, NULL);

	CloseHandle(hJK);

	return checksum;
}


CRITICAL_SECTION g_logCritSect;

void Log(const char* fmt, ...)
{
	EnterCriticalSection(&g_logCritSect);

	va_list va;
	uintptr_t msg_len;

	va_start(va, fmt);
	msg_len = vsnprintf(NULL, 0, fmt, va);
	va_end(va);

	char* buf = new char[msg_len + 1];

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);


	FILE* f = fopen(LOGPATH"directdipshit.log", "a");
	fwrite(buf, 1, msg_len, f);
	fwrite("\n", 1, 1, f);
	fclose(f);


	delete[] buf;

	LeaveCriticalSection(&g_logCritSect);
}

// super unsafe esp for threads but im lazy
const char* FormatHRESULT(HRESULT hr)
{
	_com_error err(hr);
	LPCTSTR errMsg = err.ErrorMessage();

	static char szBuf[1024];
	strncpy(szBuf, errMsg, sizeof(szBuf) - 1);

	return szBuf;//lol
}

const char* FormatDPLAYRESULT(HRESULT hr)
{
	switch (hr)
	{
	case S_OK: return "DP_OK";
	case MAKE_DPHRESULT(5): return "DPERR_ALREADYINITIALIZED";
	case MAKE_DPHRESULT(10):return "DPERR_ACCESSDENIED";
	case MAKE_DPHRESULT(20):return "DPERR_ACTIVEPLAYERS";
	case MAKE_DPHRESULT(30):return "DPERR_BUFFERTOOSMALL";
	case MAKE_DPHRESULT(40):return "DPERR_CANTADDPLAYER";
	case MAKE_DPHRESULT(50):return "DPERR_CANTCREATEGROUP";
	case MAKE_DPHRESULT(60):return "DPERR_CANTCREATEPLAYER";
	case MAKE_DPHRESULT(70):return "DPERR_CANTCREATESESSION";
	case MAKE_DPHRESULT(80):return "DPERR_CAPSNOTAVAILABLEYET";
	case MAKE_DPHRESULT(90):return "DPERR_EXCEPTION";
	case E_FAIL:return "DPERR_GENERIC";
	case MAKE_DPHRESULT(120):return "DPERR_INVALIDFLAGS";
	case MAKE_DPHRESULT(130):return "DPERR_INVALIDOBJECT";
	//case E_INVALIDARG:return "DPERR_INVALIDPARAM";
	case DPERR_INVALIDPARAM:return "DPERR_INVALIDPARAMS";
	case MAKE_DPHRESULT(150):return "DPERR_INVALIDPLAYER";
	case MAKE_DPHRESULT(155):return "DPERR_INVALIDGROUP";
	case MAKE_DPHRESULT(160):return "DPERR_NOCAPS";
	case MAKE_DPHRESULT(170):return "DPERR_NOCONNECTION";
	//case E_OUTOFMEMORY:return "DPERR_NOMEMORY";
	case DPERR_NOMEMORY:return "DPERR_OUTOFMEMORY";
	case MAKE_DPHRESULT(190):return "DPERR_NOMESSAGES";
	case MAKE_DPHRESULT(200):return "DPERR_NONAMESERVERFOUND";
	case MAKE_DPHRESULT(210):return "DPERR_NOPLAYERS";
	case MAKE_DPHRESULT(220):return "DPERR_NOSESSIONS";
	case E_PENDING:return "DPERR_PENDING";
	case MAKE_DPHRESULT(230):return "DPERR_SENDTOOBIG";
	case MAKE_DPHRESULT(240):return "DPERR_TIMEOUT";
	case MAKE_DPHRESULT(250):return "DPERR_UNAVAILABLE";
	case E_NOTIMPL:return "DPERR_UNSUPPORTED";
	case MAKE_DPHRESULT(270):return "DPERR_BUSY";
	case MAKE_DPHRESULT(280):return "DPERR_USERCANCEL";
	case E_NOINTERFACE:return "DPERR_NOINTERFACE";
	case MAKE_DPHRESULT(290):return "DPERR_CANNOTCREATESERVER";
	case MAKE_DPHRESULT(300):return "DPERR_PLAYERLOST";
	case MAKE_DPHRESULT(310):return "DPERR_SESSIONLOST";
	case MAKE_DPHRESULT(320):return "DPERR_UNINITIALIZED";
	case MAKE_DPHRESULT(330):return "DPERR_NONEWPLAYERS";
	case MAKE_DPHRESULT(340):return "DPERR_INVALIDPASSWORD";
	case MAKE_DPHRESULT(350):return "DPERR_CONNECTING";
	case MAKE_DPHRESULT(1000):return "DPERR_BUFFERTOOLARGE";
	case MAKE_DPHRESULT(1010):return "DPERR_CANTCREATEPROCESS";
	case MAKE_DPHRESULT(1020):return "DPERR_APPNOTSTARTED";
	case MAKE_DPHRESULT(1030):return "DPERR_INVALIDINTERFACE";
	case MAKE_DPHRESULT(1040):return "DPERR_NOSERVICEPROVIDER";
	case MAKE_DPHRESULT(1050):return "DPERR_UNKNOWNAPPLICATION";
	case MAKE_DPHRESULT(1070):return "DPERR_NOTLOBBIED";
	case MAKE_DPHRESULT(1080):return "DPERR_SERVICEPROVIDERLOADED";
	case MAKE_DPHRESULT(1090):return "DPERR_ALREADYREGISTERED";
	case MAKE_DPHRESULT(1100):return "DPERR_NOTREGISTERED";
	case MAKE_DPHRESULT(2000):return "DPERR_AUTHENTICATIONFAILED";
	case MAKE_DPHRESULT(2010):return "DPERR_CANTLOADSSPI";
	case MAKE_DPHRESULT(2020):return "DPERR_ENCRYPTIONFAILED";
	case MAKE_DPHRESULT(2030):return "DPERR_SIGNFAILED";
	case MAKE_DPHRESULT(2040):return "DPERR_CANTLOADSECURITYPACKAGE";
	case MAKE_DPHRESULT(2050):return "DPERR_ENCRYPTIONNOTSUPPORTED";
	case MAKE_DPHRESULT(2060):return "DPERR_CANTLOADCAPI";
	case MAKE_DPHRESULT(2070):return "DPERR_NOTLOGGEDIN";
	case MAKE_DPHRESULT(2080):return "DPERR_LOGONDENIED";

	default: return FormatHRESULT(hr);
	}
}

IDirectPlay3 *g_pDirectPlay = nullptr;

DPSESSIONDESC2* g_pSession = nullptr;

char* g_szJKLName = nullptr;


BOOL FAR PASCAL DPlayEnumSessions(
	LPCDPSESSIONDESC2 lpSessionDesc, LPDWORD lpdwTimeOut,
	DWORD dwFlags, LPVOID lpContext)
{
	HWND   hWnd = (HWND)lpContext;
	LPGUID lpGuid;
	LONG   iIndex;

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



enum ThreadState {
	Stopped,
	Starting,
	Running,
	Stopping,
};




volatile unsigned int g_uChecksum = 0;
bool g_bAttemptedJoin = false;
volatile unsigned short g_nINGAMEJOINRequestPacketNum = 0;
volatile unsigned int g_uInGameJoinPlayerToken = 0;
DPID g_hostDpid = 0;
DPID g_localPlayer = 0;// this be us
HANDLE g_hPlayerEvent = NULL;// we're usin this as the flag for thread good & ready
HANDLE g_hPlayerEventThread = NULL;
volatile ThreadState g_PlayerEventThreadState = ThreadState::Stopped;

DWORD g_uActuallyInTimestamp = 0;




class READPACKET
{
private:
	const char* raw;
	int len;
	int pos;

public:
	READPACKET(const char* _raw, int _len)
	{
		raw = _raw;
		len = _len;
		pos = 0;
	}

	int getCurrentPosition() const
	{
		return pos;
	}

	int getTotalLength() const
	{
		return len;
	}

	bool eof() const
	{
		return (pos >= len);
	}

	void skip(int n, bool breakOnNonNull=false)
	{
		if (breakOnNonNull)
		{
			for (int x = 0; x < n; x++)
				if (readByte() != 0)
					Log("RAWPACKET::skip assumed zeros but there arent zeros");// set a breakpoint here
		} else
			pos += n;
	}

	void read(void* buf, int s)
	{
		if ((pos + s) > len)
			Log("RAWPACKET::read wants to read past end of buffer");

		memcpy(buf, raw + pos, s);
		pos += s;
	}

	char readByte()
	{
		char b;
		read(&b, 1);
		return b;
	}

	short readInt16()
	{
		short i;
		read(&i, 2);
		return i;
	}

	unsigned short readUInt16()
	{
		unsigned short i;
		read(&i, 2);
		return i;
	}

	int readInt32()
	{
		int i;
		read(&i, 4);
		return i;
	}

	unsigned int readUInt32()
	{
		unsigned int i;
		read(&i, 4);
		return i;
	}

	float readFloat()
	{
		float f;
		read(&f, 4);
		return f;
	}

	void readNullTermString(char* sz, int smax)
	{
		memset(sz, 0, smax);

		for (int x = 0; x<(smax-1/*leave space for null*/); x++)
		{
			char c = readByte();
			if (c == 0)
				break;// we already memset

			sz[x] = c;
		}
	}

	void readNullTermWideString(char* sz, int max)
	{
		wchar_t* wcs = new wchar_t[max];

		int len;
		for (len = 0; len < max; len++)
		{
			wchar_t wc = (wchar_t)readUInt16();
			if (wc == 0)
				break;

			wcs[len] = wc;
		}
		wcs[len] = 0;

		wcstombs(sz, wcs, len);
		sz[len] = 0;

		delete[] wcs;
	}
};

struct WRITEPACKET
{
private:
	std::vector<char> buf;

public:
	void write(const void* p, int len)
	{
		// should optimize but im lazy
		for(int x=0; x<len; x++)
			buf.push_back(((const char*)p)[x]);
	}

	void writeByte(char c)
	{
		write(&c, 1);
	}

	void writeUInt16(unsigned short s)
	{
		write(&s, 2);
	}

	void writeInt16(short s)
	{
		write(&s, 2);
	}

	void writeInt32(int i)
	{
		write(&i, 4);
	}

	void writeUInt32(unsigned int i)
	{
		write(&i, 4);
	}

	void writeNullTermString(const char* sz)
	{
		int slen = strlen(sz);
		write(sz, slen + 1/*include null*/);
	}

	void writeNullTermWideString(const wchar_t* wcs)
	{
		int slen = wcslen(wcs);
		write(wcs, (slen + 1) * 2/*include null*/);
	}

	void writeFixedString(const char* sz, int max)
	{
		int slen = strlen(sz);
		write(sz, slen);

		for (; slen < max; slen++)
			writeByte(0);
	}

	void writeFixedWideString_MBS(const char* sz, int max)
	{
		wchar_t* wcs = new wchar_t[max];
		ZeroMemory(wcs, 2 * max);

		mbstowcs(wcs, sz, strlen(sz));

		write(wcs, 2 * max);

		delete[] wcs;
	}

	void allocateOutputBuffer(char* &pBuf, int& pLen)
	{
		pLen = buf.size();
		pBuf = new char[pLen];

		memcpy(pBuf, &buf[0], pLen);
	}

	void send(DPID destination, DPID source=(DPID)-1/*will use from our known local DPID*/, bool guaranteed = false)
	{
		char* pBuf;
		int pLen;
		allocateOutputBuffer(pBuf, pLen);

		g_pDirectPlay->Send((source == (DPID)-1) ? g_localPlayer : source, destination, (guaranteed ? DPSEND_GUARANTEED : 0), pBuf, pLen);

		delete[] pBuf;
	}

	void dump(const char* sz)
	{
		char* pBuf;
		int pLen;
		allocateOutputBuffer(pBuf, pLen);

		FILE* f = fopen(sz, "wb");
		fwrite(pBuf, 1, pLen, f);
		fclose(f);

		delete[] pBuf;
	}
};

enum MESSAGEID
{
	MSGID_ThingSync = 0x01,
	MSGID_Maybe_ConsoleMessage = 0x02,
	MSGID_Dunno_SomeSync1 = 0x04,
	MSGID_Dunno_SomeSync2 = 0x09,
	MSGID_Dunno_SomeSync3 = 0x0A,
	MSGID_Dunno_SomeSync4 = 0x0B,

	MSGID_Dunno_SomeSync5 = 0x16,
	MSGID_Dunno_SomeSync6 = 0x1D,

	MSGID_ServerInfo_PlayerList = 0x20,
	MSGID_Dunno_InGameJoin = 0x21,
	MSGID_JoinRequest = 0x22,
	MSGID_JoinRequestResponse = 0x24,
	MSGID_Dunno_PacketFlush = 0x28,// some kind of packet num sync-  seems to cause packetNum to be reset.  just includes uint16 of like known packetNum.  think its bidirectional message
	MSGID_Unknown_ServerHappy = 0x29,
	MSGID_Dunno_PlayerDamage = 0x30,
	MSGID_HostStatus = 0x31,

	MSGID_DMGORTHINGBULLSHIT = 0x3B,
	MSGID_PlayerInfo1 = 0x39,
	MSGID_PlayerInfo2 = 0x3A,
};

const char* MessageIDToString(MESSAGEID msgid)
{
	switch (msgid)
	{
	case MSGID_ThingSync: return "ThingSync";
	case MSGID_Maybe_ConsoleMessage: return "Maybe_ConsoleMessage";
	case MSGID_Dunno_SomeSync1: return "Dunno_SomeSync1";
	case MSGID_Dunno_SomeSync2: return "Dunno_SomeSync2";
	case MSGID_Dunno_SomeSync3: return "Dunno_SomeSync3";
	case MSGID_Dunno_SomeSync4: return "Dunno_SomeSync4";
	case MSGID_Dunno_SomeSync5: return "Dunno_SomeSync5";
	case MSGID_Dunno_SomeSync6: return "Dunno_SomeSync6";
	case DPSYS_CREATEPLAYERORGROUP: return "CREATEPLAYER";
	case DPSYS_DESTROYPLAYERORGROUP: return "DESTROYPLAYER";
	case MSGID_Dunno_InGameJoin: return "Dunno_InGameJoin";
	case MSGID_JoinRequest: return "JoinRequest";
	case MSGID_JoinRequestResponse: return "JoinRequestResponse";
	case MSGID_Dunno_PacketFlush: return "Dunno_PacketFlush";
	case MSGID_ServerInfo_PlayerList: return "ServerInfo_PlayerList";
	case MSGID_Unknown_ServerHappy: return "Unknown_ServerHappy";
	case MSGID_DMGORTHINGBULLSHIT: return "DMGORTHINGBULLSHIT";
	case MSGID_PlayerInfo1: return "PlayerInfo1";
	case MSGID_PlayerInfo2: return "PlayerInfo2";
	case MSGID_Dunno_PlayerDamage: return "Dunno_PlayerDamage";
	case MSGID_HostStatus: return "HostStatus";

	default:
		return "UNKNOWN";
	}
}

enum SLOTFLAG
{
	SF_CONNECTED = 0x01,
	SF_AVAILABLE = 0x02,
	SF_DATAPRESENT = 0x04,
};

const char* UnsafeTmpMBS(const wchar_t* wcs)
{
	static char sz[1024];

	int len = wcslen(wcs);
	wcstombs(sz, wcs, len);
	sz[len] = 0;

	return sz;
}

#pragma pack(push, 1)
struct JOINREQUEST
{
	char jklName[32];
	wchar_t plrName[32];
	char dunno[32];
	unsigned int checksum;
};
#pragma pack(pop)

enum JOINRESULT
{
	JR_OK = 0,
	JR_BUSY=1,
	JR_UNKNOWN=2,
	JR_CANCEL=3,
	JR_WRONGCHECKSUM = 4,
	JR_GAMEFULL = 5,
	JR_WRONGLEVEL = 6,
};

const char* JoinResultToString(JOINRESULT jr)
{
	switch (jr)
	{
	case JR_OK: return "OK";
	case JR_BUSY: return "BUSY";
	case JR_UNKNOWN: return "UNKNOWN";
	case JR_CANCEL: return "CANCEL";
	case JR_WRONGCHECKSUM: return "WRONGCHECKSUM";
	case JR_GAMEFULL: return "GAMEFULL";
	case JR_WRONGLEVEL: return "WRONGLEVEL";
	default: return "unknown";
	}
}


#pragma pack(push, 1)
struct PLAYERSLOT
{
	SLOTFLAG flags;

	// only if SF_DATAPRESENT
	DPID dpid;
	char name[16];
	char extra[9];// always zeros?

	bool isConnected() const { return (flags & SF_CONNECTED) != 0; }
	bool hasData() const { return (flags & SF_DATAPRESENT) != 0; }

	void clear()
	{
		ZeroMemory(this, sizeof(PLAYERSLOT));
	}

	void read(READPACKET& p)
	{
		flags = (SLOTFLAG)p.readUInt32();

		if (hasData())
			p.read((char*)this + 4, sizeof(PLAYERSLOT) - 4);
	}
};
#pragma pack(pop)

volatile int g_nMaxPlayerSlots = 0;
PLAYERSLOT* volatile g_pPlayerSlots = nullptr;
volatile int g_nLocalPlayerIndex = -1;

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

	bool ignoreSyncMsg = true;

	// blacklist some IDs for dev purposes to prevent flooding
	if(!ignoreSyncMsg || (msgID != MSGID_ThingSync && msgID != MSGID_Dunno_SomeSync4 && msgID != MSGID_Dunno_SomeSync5 && msgID != MSGID_Dunno_SomeSync6))
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
		int playerSlotIndex = p.readInt32();//player slot  (at least for MSGID_PlayerInfo1)

		char playerModel[32];
		p.read(playerModel, 32);

		char playerSoundClass[32];
		p.read(playerSoundClass, 32);

		char playerSaberMat0[32];
		p.read(playerSaberMat0, 32);

		char playerSaberMat1[32];
		p.read(playerSaberMat1, 32);


		Log("SOME PLAYERINFO!!  playerSlot:%d    model:%s   sndcls:%s    saber0:%s   saber1:%s", playerSlotIndex, playerModel, playerSoundClass, playerSaberMat0, playerSaberMat1);

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

		if (!ignoreSyncMsg)
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

		if (!ignoreSyncMsg)
			Log("DUNSYNC5   d1:0x%04X   d2:%d", dunno1, dunno2);
	}
	else if (msgID == MSGID_Dunno_SomeSync6)
	{
		unsigned short dunno1 = p.readUInt16();
		int dunno2 = p.readInt32();

		float f1maybe = p.readFloat();

		if (!ignoreSyncMsg)
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
		
		
		if (!ignoreSyncMsg)
			Log("DOINK1   d1:0x%04X  d2:0x%04X  d3:0x%04X  d4:0x%04X  f1:%0.02f  f2:%0.02f  f3:%0.02f  f4:%0.02f  f5:%0.02f  f6:%0.02f  f7:%0.02f  f8:%0.02f  f9:%0.02f  f10:%0.02f  ",
				dunno1, dunno2, dunno3, dunno4, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
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

DWORD WINAPI PlayerEventThread(LPVOID lpParam)
{
	g_PlayerEventThreadState = ThreadState::Running;
	while (g_PlayerEventThreadState == ThreadState::Running)
	{
		Sleep(0);//release context


		/*The message processing thread should then use the Win32 WaitForSingleObject API to wait
		until the event is set. Keep calling Receive until there are no more messages in the message queue. */
		if (WaitForSingleObject(g_hPlayerEvent, 750/*dialup days were worse*/) == 0)
		{
			// gotcha bitch- lets get ur data
			HRESULT hr;

			DPID remotePlayer=0;// host?
			DWORD msgSize = 0;
			hr = g_pDirectPlay->Receive(&remotePlayer, &g_localPlayer, DPRECEIVE_ALL, nullptr, &msgSize);
			if (hr == DPERR_NOMESSAGES)
				continue;

			if (msgSize <= 0)
			{
				Log("PLR EVENT RECEIVE WTF MSG 0 LEN  SUX DIX");
				continue;
			}

			//Log("PLAYER EVENT-  GOTTA Receive()  and see what it wants");

			char* msg = new char[msgSize];
			hr = g_pDirectPlay->Receive(&remotePlayer, &g_localPlayer, DPRECEIVE_ALL, msg, &msgSize);

			static int numreceived = 0;
			char dumpfile[MAX_PATH];
			sprintf(dumpfile, LOGPATH"dplayRECV%d.dmp", numreceived);
			FILE* f = fopen(dumpfile, "wb");
			fwrite(msg, 1, msgSize, f);
			fclose(f);


			READPACKET p(msg, msgSize);
			ProcessPacket(remotePlayer, p, numreceived);

			//Log("DUMPED NET PACKET TO: %s", dumpfile);

			numreceived++;
			delete[] msg;
		}

		// asdlkfjas nothin to do here
	}

	g_PlayerEventThreadState = ThreadState::Stopped;
	return 0;
}

void StartPlayerEventThread()
{
	// if started bail
	if (g_hPlayerEvent != NULL)
		return;

	// spawn thread
	g_PlayerEventThreadState = ThreadState::Starting;
	g_hPlayerEventThread = CreateThread(NULL, 0, PlayerEventThread, NULL, 0, NULL);
	while (g_PlayerEventThreadState == ThreadState::Starting)
		Sleep(0);

	g_hPlayerEvent = CreateEvent(NULL, TRUE, FALSE, "DDIPSHITPLR");
}

void StopPlayerEventThread()
{
	// if stopped bail
	if (g_hPlayerEvent == NULL)
		return;

	// kill thread
	if (g_PlayerEventThreadState == ThreadState::Running)
	{
		// gracefully
		g_PlayerEventThreadState = ThreadState::Stopping;
		while (g_PlayerEventThreadState != ThreadState::Stopped)
			Sleep(0);// hopefully-  could add timer
	}
	else
		TerminateThread(g_hPlayerEventThread, 0);// or not


	if (g_hPlayerEventThread != NULL)
	{
		CloseHandle(g_hPlayerEventThread);
		g_hPlayerEventThread = NULL;
	}

	if (g_hPlayerEvent != NULL)
	{
		CloseHandle(g_hPlayerEvent);
		g_hPlayerEvent = NULL;
	}
}


void CleanupOldDumpFiles()
{
	int dumpfiles = 0;

	for (int dumprun = 0; dumprun < 2; dumprun++)
	{
		WIN32_FIND_DATA fd;
		HANDLE hFind = FindFirstFile(LOGPATH"dplayRECV*.dmp", &fd);
		if (hFind != INVALID_HANDLE_VALUE && hFind != NULL)
		{
			do {
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					continue;

				if (dumprun == 0)
				{
					dumpfiles++;
				}
				else
				{
					char szDumpFile[MAX_PATH];
					sprintf(szDumpFile, LOGPATH"%s", fd.cFileName);
					DeleteFile(szDumpFile);
				}
			} while (FindNextFile(hFind, &fd));

			FindClose(hFind);

			if (dumprun == 0)
			{
				Log("CLEANUP DUMP FILES:  %d", dumpfiles);
			}
		}
	}
	
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	InitializeCriticalSection(&g_logCritSect);

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
#if true
	g_uChecksum = ExtractLocalhostChecksum();
	if (g_uChecksum != 0)
	{
		Log("I JUST STOLE THE CHECKSOME FROM JK MEMORY: %d", g_uChecksum);
	}
#endif


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
		// WARNING FOR LOCAL HOST:  if debugging multi-user sessions, cannot use 127.0.0.1  as we will not be sent other players' packets.  using the actual local adapter fixes this
		char ipAddress[] = "192.168.5.2";//"192.168.5.3";//"127.0.0.1";

		IDirectPlayLobby2* pLobby = nullptr;
		hr = CoCreateInstance(CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayLobby2, (LPVOID*)&pLobby);
		if (hr != S_OK || pLobby == nullptr)
		{
			hr = hr;
			return 0;
		}

		char* addressConnection = nullptr;
		DWORD addressConnectionLen = 0;
		pLobby->CreateAddress(DPSPGUID_TCPIP, DPAID_INet, ipAddress, strlen(ipAddress) + 1, addressConnection, &addressConnectionLen);//query size

		addressConnection = new char[addressConnectionLen];
		hr = pLobby->CreateAddress(DPSPGUID_TCPIP, DPAID_INet, ipAddress, strlen(ipAddress) + 1, addressConnection, &addressConnectionLen);
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

		StartPlayerEventThread();

		hr = g_pDirectPlay->CreatePlayer(&g_localPlayer, &localName, g_hPlayerEvent, NULL/*jk prolly wants a packet from us*/, 0/*size*/, 0/*joining-  nonserver and nonspectator*/);
		//Log("%s = DirectPlay->CreatePlayer", FormatDPLAYRESULT(hr));


		delete[] wcs;



		Log("IN SESSION.. WAITING FOREVER UNTIL U HOLD CTRL+ALT");
		for (;;)
		{
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

	StopPlayerEventThread();

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

exitmain:
	Log("----------------------------------------------");
	Log("EXITING");
	Log("----------------------------------------------");

	DeleteCriticalSection(&g_logCritSect);
	return 0;
}

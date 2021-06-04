#pragma once
#pragma pack(push, 1)


enum MESSAGEID
{
	MSGID_ThingSync = 0x01,
	MSGID_Maybe_ConsoleMessage = 0x02,
	MSGID_Dunno_SyncSectorAlt = 0x03,
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

const char* MessageIDToString(MESSAGEID msgid);

enum SLOTFLAG
{
	SF_CONNECTED = 0x01,
	SF_AVAILABLE = 0x02,
	SF_DATAPRESENT = 0x04,
};

enum JOINRESULT
{
	JR_OK = 0,
	JR_BUSY = 1,
	JR_UNKNOWN = 2,
	JR_CANCEL = 3,
	JR_WRONGCHECKSUM = 4,
	JR_GAMEFULL = 5,
	JR_WRONGLEVEL = 6,
};

const char* JoinResultToString(JOINRESULT jr);

struct JOINREQUEST
{
	char jklName[32];
	wchar_t plrName[32];
	char dunno[32];
	unsigned int checksum;
};


class PLAYERSLOT
{
public:
	SLOTFLAG flags;

	// only if SF_DATAPRESENT
	DPID dpid;
	char name[16];
	char extra[9];// always zeros?

	bool isConnected() const { return (flags & SF_CONNECTED) != 0; }
	bool hasData() const { return (flags & SF_DATAPRESENT) != 0; }

	void clear();

	void read(READPACKET& p);
};

struct DUNNOPLAYERINFO
{
	char model[32];
	char soundclass[32];
	char saber0[32];
	char saber1[32];
};






#pragma pack(pop)

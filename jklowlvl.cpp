#include "dipshit.h"
#include "jklowlvl.h"

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



void PLAYERSLOT::clear()
{
	ZeroMemory(this, sizeof(PLAYERSLOT));
}

void PLAYERSLOT::read(READPACKET& p)
{
	flags = (SLOTFLAG)p.readUInt32();

	if (hasData())
		p.read((char*)this + 4, sizeof(PLAYERSLOT) - 4);
}
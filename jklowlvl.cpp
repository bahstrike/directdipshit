#include "dipshit.h"
#include "jklowlvl.h"

const char* MessageIDToString(MESSAGEID msgid)
{
	switch (msgid)
	{
	case MSGID_ThingSync: return "ThingSync";
	case MSGID_Maybe_ConsoleMessage: return "Maybe_ConsoleMessage";
	case MSGID_Dunno_SyncSectorAlt: return "Dunno_SyncSectorAlt";
	case MSGID_Dunno_SomeSync1: return "Dunno_SomeSync1";
	case MSGID_Dunno_SomeSync2: return "Dunno_SomeSync2";
	case MSGID_Dunno_SomeSync3: return "Dunno_SomeSync3";
	case MSGID_Dunno_SomeSync4: return "Dunno_SomeSync4";
	case MSGID_Dunno_SomeSync5: return "Dunno_SomeSync5";
	case MSGID_Dunno_SomeSync6: return "Dunno_SomeSync6";
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
	{
		dpid = (DPID)p.readUInt32();
		p.read(name, 16);
		p.skip(9);// always zeros?
	}
}


void LoadLevel(const char* szEpisodeName, const char* szJKLFilename, JKTHING* &pThings, int& nNumThings, int& nMaxThings)
{
	char fullGOBPath[MAX_PATH];
	sprintf(fullGOBPath, "%s%s.gob", EPISODEPATH, szEpisodeName);

	char fullJKLPath[128];
	sprintf(fullJKLPath, "jkl\\%s", szJKLFilename);


	pThings = nullptr;
	nNumThings = 0;
	nMaxThings = 0;

	Log("Loading JKL:%s  from GOB: %s", fullJKLPath, fullGOBPath);

	FILE* gob = fopen(fullGOBPath, "rb");

	fseek(gob, 12, SEEK_CUR);
	int numEntries;
	fread(&numEntries, 4, 1, gob);

	for (int x = 0; x < numEntries; x++)
	{
		unsigned int offset;
		unsigned int length;
		char filepath[128];

		fread(&offset, 4, 1, gob);
		fread(&length, 4, 1, gob);
		fread(filepath, 1, 128, gob);

		if (!_stricmp(fullJKLPath, filepath))
		{
			// found target JKL-  lets load it
			fseek(gob, offset, SEEK_SET);

			int foundthings = 0;
			for (; ;)
			{
				char line[512];
				fscanf(gob, "%[^\n] ", line);
				for (int i = 0; line[i]; i++)
					line[i] = tolower(line[i]);

				switch (foundthings)
				{
					case 0:
					{
						char what[128];
						int ret = sscanf(line, "section: %s", what);

						if (ret != 0 && !strcmp(what, "things"))
							foundthings = 1;
					}
					break;

					case 1:
					{
						if (sscanf(line, "world things %d", &nMaxThings) != 0)
						{
							pThings = new JKTHING[nMaxThings];
							ZeroMemory(pThings, nMaxThings * sizeof(JKTHING));
							foundthings = 2;
						}
					}
					break;

					case 2:
					{
						JKTHING thing;
						ZeroMemory(&thing, sizeof(JKTHING));

						int nonWS = 0;
						for (; line[nonWS] && (line[nonWS] == ' ' || line[nonWS] == '\t'); nonWS++);

						if (!strncmp(&line[nonWS], "end", 3))
							foundthings = 3;// done
						else {
							int numTokens = sscanf(line, "%d: %s %s %f %f %f %f %f %f %d", &thing.thingNum, thing.tplName, thing.name, &thing.px, &thing.py, &thing.pz, &thing.pitch, &thing.yaw, &thing.roll, &thing.sector);
							if (numTokens == 10)
								memcpy(&pThings[nNumThings++], &thing, sizeof(JKTHING));
						}
					}
					break;
				}

				if (foundthings == 3)
					break;
			}

			// loaded our target data- we are done
			break;
		}
	}

	fclose(gob);
}
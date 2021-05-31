#include "dipshit.h"

//#pragma comment(lib, "cmcfg32.lib")// need for priv stuff?  maybe not

GUID* GimmeJKGUID()
{
	static unsigned int _jkguid[] = {
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



unsigned int ExtractLocalhostChecksum(void* pChecksumAddress)
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
	ReadProcessMemory(hJK, pChecksumAddress, &checksum, 4, NULL);

	CloseHandle(hJK);

	return checksum;
}
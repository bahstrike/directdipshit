#include "dipshit.h"


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


	FILE* f = fopen(GenerateLogFilePath("directdipshit.log"), "a");
	fwrite(buf, 1, msg_len, f);
	fwrite("\n", 1, 1, f);
	fclose(f);


	delete[] buf;

	LeaveCriticalSection(&g_logCritSect);
}

void LogInit()
{
	InitializeCriticalSection(&g_logCritSect);
}


void LogShutdown()
{
	DeleteCriticalSection(&g_logCritSect);
}

const char* GenerateLogFilePath(const char* szFileName)
{
	static char path[MAX_PATH];

	strcpy(path, LOGPATH);
	strcat(path, "\\");
	strcat(path, szFileName);

	return path;
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
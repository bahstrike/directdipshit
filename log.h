#pragma once


// thread-safe for appending to logfile
void Log(const char* fmt, ...);


void LogInit();
void LogShutdown();


void CleanupOldDumpFiles();

// these functions may return ptrs from static buffers;  use with caution
const char* GenerateLogFilePath(const char* szFileName);
const char* FormatHRESULT(HRESULT hr);
const char* FormatDPLAYRESULT(HRESULT hr);
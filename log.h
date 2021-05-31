#pragma once

// Log()  should be pretty much thread-safe, for appending to a file.
//

void Log(const char* fmt, ...);


void LogInit();
void LogShutdown();


// super unsafe esp for threads but im lazy
const char* FormatHRESULT(HRESULT hr);

const char* FormatDPLAYRESULT(HRESULT hr);
#include "dipshit.h"


extern IDirectPlay3 *g_pDirectPlay;// we cheatin some
extern DPID g_localPlayer;// we cheatin some



READPACKET::READPACKET(const char* _raw, int _len)
{
	raw = _raw;
	len = _len;
	pos = 0;
}

int READPACKET::getCurrentPosition() const
{
	return pos;
}

int READPACKET::getTotalLength() const
{
	return len;
}

bool READPACKET::eof() const
{
	return (pos >= len);
}

void READPACKET::skip(int n, bool breakOnNonNull)
{
	if (breakOnNonNull)
	{
		for (int x = 0; x < n; x++)
			if (readByte() != 0)
				Log("RAWPACKET::skip assumed zeros but there arent zeros");// set a breakpoint here
	}
	else
		pos += n;
}

void READPACKET::read(void* buf, int s)
{
	if ((pos + s) > len)
		Log("RAWPACKET::read wants to read past end of buffer");

	memcpy(buf, raw + pos, s);
	pos += s;
}

char READPACKET::readByte()
{
	char b;
	read(&b, 1);
	return b;
}

short READPACKET::readInt16()
{
	short i;
	read(&i, 2);
	return i;
}

unsigned short READPACKET::readUInt16()
{
	unsigned short i;
	read(&i, 2);
	return i;
}

int READPACKET::readInt32()
{
	int i;
	read(&i, 4);
	return i;
}

unsigned int READPACKET::readUInt32()
{
	unsigned int i;
	read(&i, 4);
	return i;
}

float READPACKET::readFloat()
{
	float f;
	read(&f, 4);
	return f;
}

void READPACKET::readNullTermString(char* sz, int smax)
{
	memset(sz, 0, smax);

	for (int x = 0; x < (smax - 1/*leave space for null*/); x++)
	{
		char c = readByte();
		if (c == 0)
			break;// we already memset

		sz[x] = c;
	}
}

void READPACKET::readNullTermWideString(char* sz, int max)
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





void WRITEPACKET::write(const void* p, int len)
{
	// should optimize but im lazy
	for (int x = 0; x < len; x++)
		buf.push_back(((const char*)p)[x]);
}

void WRITEPACKET::writeByte(char c)
{
	write(&c, 1);
}

void WRITEPACKET::writeUInt16(unsigned short s)
{
	write(&s, 2);
}

void WRITEPACKET::writeInt16(short s)
{
	write(&s, 2);
}

void WRITEPACKET::writeInt32(int i)
{
	write(&i, 4);
}

void WRITEPACKET::writeUInt32(unsigned int i)
{
	write(&i, 4);
}

void WRITEPACKET::writeFloat(float f)
{
	write(&f, 4);
}

void WRITEPACKET::writeNullTermString(const char* sz)
{
	int slen = strlen(sz);
	write(sz, slen + 1/*include null*/);
}

void WRITEPACKET::writeNullTermWideString(const wchar_t* wcs)
{
	int slen = wcslen(wcs);
	write(wcs, (slen + 1) * 2/*include null*/);
}

void WRITEPACKET::writeFixedString(const char* sz, int max)
{
	int slen = strlen(sz);
	write(sz, slen);

	for (; slen < max; slen++)
		writeByte(0);
}

void WRITEPACKET::writeFixedWideString_MBS(const char* sz, int max)
{
	wchar_t* wcs = new wchar_t[max];
	ZeroMemory(wcs, 2 * max);

	mbstowcs(wcs, sz, strlen(sz));

	write(wcs, 2 * max);

	delete[] wcs;
}

void WRITEPACKET::allocateOutputBuffer(char* &pBuf, int& pLen)
{
	pLen = buf.size();
	pBuf = new char[pLen];

	memcpy(pBuf, &buf[0], pLen);
}

void WRITEPACKET::send(DPID destination, DPID source, bool guaranteed)
{
	char* pBuf;
	int pLen;
	allocateOutputBuffer(pBuf, pLen);

	g_pDirectPlay->Send((source == (DPID)-1) ? g_localPlayer : source, destination, (guaranteed ? DPSEND_GUARANTEED : 0), pBuf, pLen);

	delete[] pBuf;
}

void WRITEPACKET::dump(const char* sz)
{
	char* pBuf;
	int pLen;
	allocateOutputBuffer(pBuf, pLen);

	FILE* f = fopen(sz, "wb");
	fwrite(pBuf, 1, pLen, f);
	fclose(f);

	delete[] pBuf;
}
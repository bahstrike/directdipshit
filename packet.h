#pragma once




class READPACKET
{
private:
	const char* raw;
	int len;
	int pos;

public:
	READPACKET(const char* _raw, int _len);
	int getCurrentPosition() const;
	int getTotalLength() const;
	bool eof() const;
	void skip(int n, bool breakOnNonNull = false);
	void read(void* buf, int s);
	char readByte();
	short readInt16();
	unsigned short readUInt16();
	int readInt32();
	unsigned int readUInt32();
	float readFloat();
	void readNullTermString(char* sz, int smax);
	void readNullTermWideString(char* sz, int max);
};

class WRITEPACKET
{
private:
	std::vector<char> buf;

public:
	void write(const void* p, int len);
	void writeByte(char c);
	void writeUInt16(unsigned short s);
	void writeInt16(short s);
	void writeInt32(int i);
	void writeUInt32(unsigned int i);
	void writeFloat(float f);
	void writeNullTermString(const char* sz);
	void writeNullTermWideString(const wchar_t* wcs);
	void writeFixedString(const char* sz, int max);
	void writeFixedWideString_MBS(const char* sz, int max);
	void allocateOutputBuffer(char* &pBuf, int& pLen);
	void send(DPID destination, DPID source = (DPID)-1/*will use from our known local DPID*/, bool guaranteed = false);
	void dump(const char* sz);
};
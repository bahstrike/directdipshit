#pragma once


GUID* GimmeJKGUID();



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


// TO BE USED WHEN HOSTING JK LOCALLY-  just get the checksum value from the process directly.
// IF THIS SHIT FAILS, YOU NEED TO RUN VEGETABLESTUDIO AS ADMIN
unsigned int ExtractLocalhostChecksum(void* pChecksumAddress=(void*)0x00832678/*might have to change this if not running CD copy*/);
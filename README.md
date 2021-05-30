# directdipshit
netcode haxx for jkdf2

# project setup
**requires some manual edits**

1. launch vegetablestudio as admin  *(required if u want to debug and memory-scrape checksum from a locally-hosted game)*
2. change/set up  ur inc/lib paths for directx  *(locally compiled with dx5.2sdk;  not included.. source it urself)*
3. edit `#define LOGPATH "c:\\directdipshit\\"`  *(if u have checked out to a diff location)*
4. change `char ipAddress[]`  to:   ur localadapter, if hosting...    *otherwise set to remote IP and disable `g_uChecksum = ExtractLocalhostChecksum();`.  program will wait until someone else joins, to steal checksum value...*

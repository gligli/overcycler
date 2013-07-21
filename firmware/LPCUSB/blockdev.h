#include "type.h"


int BlockDevInit(void);

int BlockDevWrite(U32 dwAddress, U8* pbBuf);
int BlockDevRead(U32 dwAddress, U8* pbBuf);

int BlockDevGetSize(U32 *pdwDriveSize);
int BlockDevGetStatus(void);

#include "type.h"

void	SCSIReset(void);
U8 *	SCSIHandleCmd(U8 *pbCDB, int iCDBLen, int *piRspLen, BOOL *pfDevIn);
U8 *	SCSIHandleData(U8 *pbCDB, int iCDBLen, U8 *pbData, U32 dwOffset);

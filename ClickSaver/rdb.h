#ifndef RDB_H
#define RDB_H

#include "sqlite3.h"
#include "Platform.h" 
#include "mission.h" 

#ifndef AODB_TYP_ITEM
#define AODB_TYP_ITEM 1000020
#define AODB_TYP_ICON 1010008
#define AODB_TYP_PF   1000001
#endif

extern sqlite3*      g_pSQLite;
extern sqlite3_stmt* g_stmtItem;
extern sqlite3_stmt* g_stmtIcon;
extern sqlite3_stmt* g_stmtPF;
extern char          g_AODir[MAX_PATH];

int   OpenLocalDB(void);
void* GetDataChunk(PUU32 _KeyHi, PUU32 _KeyLo, PUU32* _pSize);
void  ReleaseAODatabase(void);

#endif // RDB_H


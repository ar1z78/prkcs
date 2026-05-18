#ifndef AOMD_H
#define AOMD_H

#include <windows.h>
#include "Platform.h" // Guarantees basic types like PUU32, PUS32, and PUU8 exist
#include "mission.h"  // Ensures MissionItem layout data definition is pulled in

// Function declarations
PUU8 *GetAOIconData(unsigned long lIconNo);
void GetMissionItem(MissionItem* _pMissionItem, PUU32 _ItemKey1, PUU32 _ItemKey2, PUU32 _QL);
PUU8 GetAODBItem(MissionItem* _pMissionItem, PUU32 _ItemKey);
void MissionPF(PUS32 _PFNum, PUU8* _pPFString);

#endif // AOMD_H

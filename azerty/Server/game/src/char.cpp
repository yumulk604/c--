#include "log.h"
#include "motion.h"
#include "over9refine.h"
#include "realtime_manager.h"

extern int passes_per_sec;
extern void SendShout(const char * szText, BYTE bEmpire);

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

	// Clear realtime subscriptions
	CRealtimeManager::instance().ClearSubscriptions(GetPlayerID());

	if (m_bMonsterLog)
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);

	ViewCleanup();

	CEntity::Destroy();
}

void CHARACTER::SendRealtimeEvent(CRealtimeManager::ERealtimeEventType type, 
                                  DWORD dwTargetID, DWORD dwData1, DWORD dwData2, 
                                  const std::string& strData)
{
	if (!IsPC())
		return;

	CRealtimeManager::instance().AddEvent(type, GetPlayerID(), dwTargetID, 
	                                      GetX(), GetY(), dwData1, dwData2, strData);
}

	if (GetMapIndex() != lMapIndex)
		return false;

	// Send realtime move event
	SendRealtimeEvent(CRealtimeManager::REALTIME_PLAYER_MOVE, 0, x, y);

	Stop();

	Initialize();
#include "affect_flag.h"
#include "cube.h"
#include "mining.h"
#include "realtime_manager.h"

class CBuffOnAttributes;
class CPetSystem;

		void			SetLastAttacker(LPCHARACTER pkChrAttacker);
		LPCHARACTER		GetLastAttacker() const { return m_pkChrLastAttacker; }

		// Realtime system
		void			SendRealtimeEvent(CRealtimeManager::ERealtimeEventType type, 
							DWORD dwTargetID = 0, DWORD dwData1 = 0, DWORD dwData2 = 0, 
							const std::string& strData = "");

	protected:
		// CHARACTER_AFFECT
		void			LoadAffect(DWORD dwCount, TPacketAffectElement * pElements);
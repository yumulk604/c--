#include "log.h"
#include "motion.h"
#include "over9refine.h"
#include "realtime_manager.h"

extern int passes_per_sec;
extern void SendShout(const char * szText, BYTE bEmpire);

	if (battle_melee_attack(this, pkVictim))
		return true;

	// Send realtime attack event
	SendRealtimeEvent(CRealtimeManager::REALTIME_PLAYER_ATTACK, 
	                  pkVictim->GetVID(), GetPoint(POINT_ATT_GRADE), 0);

	return false;
}

	if (dwVnum == 0)
		return false;

	// Send realtime skill event
	SendRealtimeEvent(CRealtimeManager::REALTIME_PLAYER_SKILL, 
	                  pkVictim ? pkVictim->GetVID() : 0, dwVnum, bSkillLevel);

	if (m_SkillUseInfo[dwVnum].bUsed && m_SkillUseInfo[dwVnum].dwNextSkillUsableTime > dwCur)
	{
		sys_log(0, "SKILL_HACK: %s %d %d", GetName(), dwVnum, dwCur - m_SkillUseInfo[dwVnum].dwNextSkillUsableTime);
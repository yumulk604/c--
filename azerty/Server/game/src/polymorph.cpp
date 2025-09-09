#include "stdafx.h"
#include "char.h"
#include "mob_manager.h"
#include "affect.h"
#include "item.h"
#include "polymorph.h"
#include "questmanager.h"
#include "item_manager.h"

CPolymorphUtils::CPolymorphUtils()
{
	m_mapSPDType.insert(std::make_pair(101, 101));
	m_mapSPDType.insert(std::make_pair(1901, 1901));
}

POLYMORPH_BONUS_TYPE CPolymorphUtils::GetBonusType(DWORD dwVnum)
{
	boost::unordered_map<DWORD, DWORD>::iterator iter;

	iter = m_mapSPDType.find(dwVnum);

	if (iter != m_mapSPDType.end())
		return POLYMORPH_SPD_BONUS;

	iter = m_mapATKType.find(dwVnum);

	if (iter != m_mapATKType.end())
		return POLYMORPH_ATK_BONUS;

	iter = m_mapDEFType.find(dwVnum);

	if (iter != m_mapDEFType.end())
		return POLYMORPH_DEF_BONUS;

	return POLYMORPH_NO_BONUS;
}

bool CPolymorphUtils::PolymorphCharacter(LPCHARACTER pChar, LPITEM pItem, const CMob* pMob)
{
	BYTE bySkillLevel = pChar->GetSkillLevel(POLYMORPH_SKILL_ID);
	DWORD dwDuration = 0;
	DWORD dwBonusPercent = 0;
	int iPolyPercent = 0;

	switch (pChar->GetSkillMasterType(POLYMORPH_SKILL_ID))
	{
		case SKILL_NORMAL:
			dwDuration = 10;
			break;

		case SKILL_MASTER:
			dwDuration = 15;
			break;

		case SKILL_GRAND_MASTER:
			dwDuration = 20;
			break;

		case SKILL_PERFECT_MASTER:
			dwDuration = 25;
			break;

		default:
			return false;
	}

	// dwDuration *= 60;

	iPolyPercent = pChar->GetLevel() - pMob->m_table.bLevel + pItem->GetSocket(2) + (29 + bySkillLevel);

	if (iPolyPercent <= 0)
	{
		pChar->ChatPacket(CHAT_TYPE_INFO, "Nu m-am transformat.");
		return false;
	}
	else
	{
		if (number(1, 100) > iPolyPercent)
		{
			pChar->ChatPacket(CHAT_TYPE_INFO, "Nu m-am transformat.");
			return false;
		}
	}

	pChar->AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, pMob->m_table.dwVnum, AFF_POLYMORPH, dwDuration, 0, true);

	dwBonusPercent = bySkillLevel + pItem->GetSocket(2);

	switch (GetBonusType(pMob->m_table.dwVnum))
	{
		case POLYMORPH_ATK_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		case POLYMORPH_DEF_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_DEF_BONUS, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		case POLYMORPH_SPD_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_MOV_SPEED, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		default:
		case POLYMORPH_NO_BONUS:
			break;
	}

	return true;
}

bool CPolymorphUtils::UpdateBookPracticeGrade(LPCHARACTER pChar, LPITEM pItem)
{
	if (pChar == NULL || pItem == NULL)
		return false;

	if (pItem->GetSocket(1) > 0)
		pItem->SetSocket(1, pItem->GetSocket(1) - 1);
	else
		pChar->ChatPacket(CHAT_TYPE_INFO, "Cibirichi48");

	return true;
}

bool CPolymorphUtils::GiveBook(LPCHARACTER pChar, DWORD dwMobVnum, DWORD dwPracticeCount, BYTE BookLevel, BYTE LevelLimit)
{
	if (!pChar)
		return false;

	LPITEM pItem = pChar->AutoGiveItem(POLYMORPH_BOOK_ID, 1);
	if (!pItem)
		return false;

	if (CMobManager::instance().Get(dwMobVnum) == NULL)
	{
		sys_err("Wrong Polymorph vnum passed: CPolymorphUtils::GiveBook(PID(%d), %d %d %d %d)", 
				pChar->GetPlayerID(), dwMobVnum, dwPracticeCount, BookLevel, LevelLimit);
		return false;
	}

	pItem->SetSocket(0, dwMobVnum);
	pItem->SetSocket(1, dwPracticeCount);
	pItem->SetSocket(2, BookLevel);
	return true;
}

bool CPolymorphUtils::BookUpgrade(LPCHARACTER pChar, LPITEM pItem)
{
	if (!pChar || !pItem)
		return false;

	pItem->SetSocket(1, pItem->GetSocket(2) * 50);
	pItem->SetSocket(2, pItem->GetSocket(2)+1);
	return true;
}

bool CPolymorphUtils::ShouldDropPolymorphBook(LPCHARACTER pkKiller, LPCHARACTER pkVictim)
{
	if (!pkKiller || !pkVictim)
		return false;

	// Check if polymorph drop event is enabled
	if (!quest::CQuestManager::instance().GetEventFlag(POLYMORPH_DROP_EVENT_FLAG))
		return false;

	// Check minimum level requirement
	if (pkKiller->GetLevel() < POLYMORPH_DROP_MIN_LEVEL)
		return false;

	// Only drop from monsters, not players
	if (pkVictim->IsPC())
		return false;

	// Check if victim is a valid polymorph target
	if (pkVictim->GetMobRank() < MOB_RANK_KNIGHT)
		return false;

	// Calculate drop chance
	int iDropPercent = GetDropPercent(pkKiller, pkVictim);
	
	return number(1, 10000) <= iDropPercent;
}

DWORD CPolymorphUtils::GetPolymorphBookVnum(LPCHARACTER pkVictim)
{
	if (!pkVictim)
		return 0;

	// Return the polymorph book for this specific monster
	return POLYMORPH_BOOK_ID;
}

int CPolymorphUtils::GetDropPercent(LPCHARACTER pkKiller, LPCHARACTER pkVictim)
{
	if (!pkKiller || !pkVictim)
		return 0;

	int iBasePct = POLYMORPH_DROP_BASE_PCT;
	
	// Get event flag multiplier
	int iEventMultiplier = quest::CQuestManager::instance().GetEventFlag(POLYMORPH_DROP_EVENT_FLAG);
	if (iEventMultiplier <= 0)
		return 0;

	// Level difference bonus/penalty
	int iLevelDiff = pkVictim->GetLevel() - pkKiller->GetLevel();
	int iLevelBonus = 0;
	
	if (iLevelDiff >= 10)
		iLevelBonus = 50; // 50% bonus for higher level monsters
	else if (iLevelDiff >= 5)
		iLevelBonus = 25; // 25% bonus
	else if (iLevelDiff < -10)
		iLevelBonus = -50; // 50% penalty for much lower level monsters

	// Mob rank bonus
	int iRankBonus = 0;
	switch (pkVictim->GetMobRank())
	{
		case MOB_RANK_KNIGHT:
			iRankBonus = 10;
			break;
		case MOB_RANK_BOSS:
			iRankBonus = 50;
			break;
		case MOB_RANK_KING:
			iRankBonus = 100;
			break;
		default:
			break;
	}

	// Calculate final percentage
	int iFinalPct = (iBasePct + iLevelBonus + iRankBonus) * iEventMultiplier / 100;
	
	return MINMAX(1, iFinalPct, 1000); // Cap between 0.01% and 10%
}

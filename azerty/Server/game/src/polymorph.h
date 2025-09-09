
#ifndef __POLYMORPH_UTILS__
#define __POLYMORPH_UTILS__

#include <boost/unordered_map.hpp>

#define POLYMORPH_SKILL_ID	129
#define POLYMORPH_BOOK_ID	50322

// Polymorph drop system
#define POLYMORPH_DROP_EVENT_FLAG "polymorph_drop"
#define POLYMORPH_DROP_MIN_LEVEL 30
#define POLYMORPH_DROP_BASE_PCT 100

enum POLYMORPH_BONUS_TYPE
{
	POLYMORPH_NO_BONUS,
	POLYMORPH_ATK_BONUS,
	POLYMORPH_DEF_BONUS,
	POLYMORPH_SPD_BONUS,
};

class CPolymorphUtils : public singleton<CPolymorphUtils>
{
	private :
		boost::unordered_map<DWORD, DWORD> m_mapSPDType;
		boost::unordered_map<DWORD, DWORD> m_mapATKType;
		boost::unordered_map<DWORD, DWORD> m_mapDEFType;

	public :
		CPolymorphUtils();

		POLYMORPH_BONUS_TYPE GetBonusType(DWORD dwVnum);

		bool PolymorphCharacter(LPCHARACTER pChar, LPITEM pItem, const CMob* pMob);
		bool UpdateBookPracticeGrade(LPCHARACTER pChar, LPITEM pItem);
		bool GiveBook(LPCHARACTER pChar, DWORD dwMobVnum, DWORD dwPracticeCount, BYTE BookLevel, BYTE LevelLimit);
		bool BookUpgrade(LPCHARACTER pChar, LPITEM pItem);

		// Drop system
		bool ShouldDropPolymorphBook(LPCHARACTER pkKiller, LPCHARACTER pkVictim);
		DWORD GetPolymorphBookVnum(LPCHARACTER pkVictim);
		int GetDropPercent(LPCHARACTER pkKiller, LPCHARACTER pkVictim);
};

#endif /*__POLYMORPH_UTILS__*/

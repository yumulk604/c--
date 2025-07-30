#ifndef __INC_METIN_II_KINGDOM_H__
#define __INC_METIN_II_KINGDOM_H__

#include "char.h"

class CKingdom
{
public:
    CKingdom(DWORD id, DWORD owner_id, const char* name, long map_index, long x, long y);
    ~CKingdom();

    DWORD GetID() const { return m_id; }
    DWORD GetOwnerID() const { return m_owner_id; }
    const char* GetName() const { return m_name; }
    long GetMapIndex() const { return m_map_index; }
    long GetX() const { return m_x; }
    long GetY() const { return m_y; }
    time_t GetLastTaxDate() const { return m_last_tax_date; }

    void SetName(const char* name);
    void SetMapIndex(long map_index);
    void SetX(long x);
    void SetY(long y);
    void SetLastTaxDate(time_t date);

    void AddNPC(DWORD npc_vnum, long x, long y);
    void RemoveNPC(DWORD npc_id);
    void UpdateNPC(DWORD npc_id, long x, long y);

private:
    DWORD m_id;
    DWORD m_owner_id;
    char m_name[50];
    long m_map_index;
    long m_x;
    long m_y;
    time_t m_last_tax_date;

    std::map<DWORD, TKingdomNPC> m_npcs;
};

#endif // __INC_METIN_II_KINGDOM_H__

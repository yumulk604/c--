// vim: ts=8 sw=4
#ifndef __INC_PEER_H__
#define __INC_PEER_H__

#include <vector>
#include "PeerBase.h"

class CPeer : public CPeerBase
{
    protected:
	virtual void OnAccept();
	virtual void OnClose();
	virtual void OnConnect();

    public:
#pragma pack(1)
	typedef struct _header
	{   
	    BYTE    bHeader;
	    DWORD   dwHandle;
	    DWORD   dwSize;
	} HEADER;
#pragma pack()
	enum EState
	{
	    STATE_CLOSE = 0,
	    STATE_PLAYING = 1
	};

	CPeer();
	virtual ~CPeer();

	void	EncodeHeader(BYTE header, DWORD dwHandle, DWORD dwSize);
	bool 	PeekPacket(int & iBytesProceed, BYTE & header, DWORD & dwHandle, DWORD & dwLength, const char ** data);
	void	EncodeReturn(BYTE header, DWORD dwHandle);

	void	ProcessInput();
	int	Send();

	DWORD	GetHandle();
	DWORD	GetUserCount();
	void	SetUserCount(DWORD dwCount);

	void	SetPublicIP(const char * ip)	{ m_stPublicIP = ip; }
	const char * GetPublicIP()		{ return m_stPublicIP.c_str(); }

	void	SetChannel(BYTE bChannel)	{ m_bChannel = bChannel; }
	BYTE	GetChannel()			{ return m_bChannel; }

	void	SetListenPort(WORD wPort) { m_wListenPort = wPort; }
	WORD	GetListenPort() { return m_wListenPort; }

	void	SetP2PPort(WORD wPort);
	WORD	GetP2PPort() { return m_wP2PPort; }

        void    SetMaps(const std::vector<long>& maps);
        const std::vector<long>& GetMaps() const { return m_vecMaps; }

	bool	SetItemIDRange(TItemIDRangeTable itemRange);
	bool	SetSpareItemIDRange(TItemIDRangeTable itemRange);
	bool	CheckItemIDRangeCollision(TItemIDRangeTable itemRange);
	void	SendSpareItemIDRange();

    private:
	int	m_state;

	BYTE	m_bChannel;
	DWORD	m_dwHandle;
	DWORD	m_dwUserCount;
	WORD	m_wListenPort;	// ÃÂ°ÃÂÃÂÃÂÃÂ¼ÃÂ­ÃÂ¹ÃÂ¶ÃÂ°ÃÂ¡ Ã
ÃÂ¬ÃÂ¶ÃÂ³ÃÂÃÂÃÂ¾ÃÂ°ÃÂÃÂ®ÃÂ¸ÃÂ¦ ÃÂÃÂ§ÃÂÃÂ listen ÃÂÃÂÃÂ´ÃÂ ÃÂÃÂ·ÃÂÃÂ®
	WORD	m_wP2PPort;	// ÃÂ°ÃÂÃÂÃÂÃÂ¼ÃÂ­ÃÂ¹ÃÂ¶ÃÂ°ÃÂ¡ ÃÂ°ÃÂÃÂÃÂÃÂ¼ÃÂ­ÃÂ¹ÃÂ¶ P2P ÃÂÃÂ¢ÃÂ¼ÃÂÃÂÃÂ» ÃÂÃÂ§ÃÂÃÂ listen ÃÂÃÂÃÂ´ÃÂ ÃÂÃÂ·ÃÂÃÂ®
        std::vector<long> m_vecMaps;

	TItemIDRangeTable m_itemRange;
	TItemIDRangeTable m_itemSpareRange;

	std::string m_stPublicIP;
};

#endif

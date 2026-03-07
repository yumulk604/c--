#include "stdafx.h" 
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "locale_service.h"
#include "db.h"
#include "limit_time.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

extern time_t get_global_time();

namespace
{

std::string Base64UrlDecode(const std::string& input)
{
        static const std::string base64_chars =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "0123456789+/";

        std::string padded = input;
        std::replace(padded.begin(), padded.end(), '-', '+');
        std::replace(padded.begin(), padded.end(), '_', '/');

        while (padded.size() % 4)
                padded.push_back('=');

        std::string output;
        std::vector<int> char_value(256, -1);
        for (size_t i = 0; i < base64_chars.size(); ++i)
                char_value[static_cast<size_t>(base64_chars[i])] = static_cast<int>(i);

        int val = 0;
        int bits = -8;
        for (unsigned char c : padded)
        {
                if (char_value[c] == -1)
                        continue;

                val = (val << 6) + char_value[c];
                bits += 6;

                if (bits >= 0)
                {
                        output.push_back(static_cast<char>((val >> bits) & 0xFF));
                        bits -= 8;
                }
        }

        return output;
}

bool ExtractJsonStringField(const std::string& json, const std::string& key, std::string& out)
{
        const std::string pattern = "\"" + key + "\"";
        size_t start = json.find(pattern);
        if (start == std::string::npos)
                return false;

        start = json.find('"', start + pattern.size());
        if (start == std::string::npos)
                return false;

        size_t end = json.find('"', start + 1);
        if (end == std::string::npos || end <= start + 1)
                return false;

        out.assign(json.begin() + static_cast<std::string::difference_type>(start + 1),
                   json.begin() + static_cast<std::string::difference_type>(end));
        return true;
}

bool ParseGoogleIdToken(const std::string& token, std::string& email, std::string& subject)
{
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot == std::string::npos ? 0 : first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos)
                return false;

        const std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        const std::string decoded = Base64UrlDecode(payload);

        if (!ExtractJsonStringField(decoded, "email", email))
                return false;

        if (!ExtractJsonStringField(decoded, "sub", subject))
                return false;

        return true;
}

std::string NormalizeGoogleLogin(const std::string& email)
{
        std::string normalized = email;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), tolower);
        return normalized;
}

bool EnsureGoogleAccount(const std::string& email, const std::string& googlePassword)
{
        char escapedLogin[LOGIN_MAX_LEN * 2 + 1];
        char escapedPassword[PASSWD_MAX_LEN * 2 + 1];

        DBManager::instance().EscapeString(escapedLogin, sizeof(escapedLogin), email.c_str(), email.size());
        DBManager::instance().EscapeString(escapedPassword, sizeof(escapedPassword), googlePassword.c_str(), googlePassword.size());

        std::unique_ptr<SQLMsg> selectMsg(DBManager::instance().DirectQuery(
                "SELECT id FROM account WHERE login='%s'", escapedLogin));

        if (!selectMsg || !selectMsg->Get())
                return false;

        if (selectMsg->Get()->uiNumRows > 0)
        {
                        std::unique_ptr<SQLMsg> updateMsg(DBManager::instance().DirectQuery(
                        "UPDATE account SET password=PASSWORD('%s') WHERE login='%s'",
                        escapedPassword, escapedLogin));
                return updateMsg && updateMsg->Get();
        }

        std::unique_ptr<SQLMsg> insertMsg(DBManager::instance().DirectQuery(
                "INSERT INTO account (login,password,status,create_time) VALUES('%s', PASSWORD('%s'), 'OK', NOW())",
                escapedLogin, escapedPassword));

        return insertMsg && insertMsg->Get();
}

}

bool FN_IS_VALID_LOGIN_STRING(const char *str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		// ���ĺ��� ���ڸ� ���
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
}

void CInputAuth::Login(LPDESC d, const char * c_pData)
{
	extern bool Metin2Server_IsInvalid();

#ifdef ENABLE_LIMIT_TIME
	if (Metin2Server_IsInvalid())
	{
		exit(1);
		return;
	}
#endif
	TPacketCGLogin3 * pinfo = (TPacketCGLogin3 *) c_pData;

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	// string ���Ἲ�� ���� ����
	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	bool bGoogleLogin = false;
	std::string googleEmail;
	std::string googleSubject;

	if (strchr(passwd, '.') && ParseGoogleIdToken(passwd, googleEmail, googleSubject))
	{
		const std::string normalized = NormalizeGoogleLogin(googleEmail);
		std::string googlePassword = "google-" + googleSubject;

		if (!EnsureGoogleAccount(normalized, googlePassword))
		{
			sys_log(0, "InputAuth::Login : GOOGLE_REGISTER_FAIL(%s) desc %p", normalized.c_str(), get_pointer(d));
			LoginFailure(d, "NOID");
			return;
		}

		strlcpy(login, normalized.c_str(), sizeof(login));
		strlcpy(passwd, googlePassword.c_str(), sizeof(passwd));
		bGoogleLogin = true;
	}

	sys_log(0, "InputAuth::Login : %s(%d) desc %p",
			login, strlen(login), get_pointer(d));

	// check login string
	if (!bGoogleLogin && false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);

	TPacketCGLogin3 * p = M2_NEW TPacketCGLogin3;
	thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	// CHANNEL_SERVICE_LOGIN
	if (Login_IsInChannelService(szLogin))
	{
		sys_log(0, "ChannelServiceLogin [%s]", szLogin);

		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT '%s',password,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				szPasswd, szLogin);
	}
	// END_OF_CHANNEL_SERVICE_LOGIN
	else
	{
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p, 
				"SELECT PASSWORD('%s'),password,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				szPasswd, szLogin);
	}
}

extern void socket_timeout(socket_t s, long sec, long usec);

int CInputAuth::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (!g_bAuthServer)
	{
		sys_err("CInputAuth class is not for game server. IP %s might be a hacker.", inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}
	
	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_LOGIN3:
			Login(d, c_pData);
			break;

		case HEADER_CG_HANDSHAKE:
			break;

		default:
			sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)", bHeader, bHeader);
			break;
	}

	return iExtraLen;
}

#ifndef __LOCALE_SERVCICE__
#define __LOCALE_SERVCICE__

bool LocaleService_Init(const std::string& c_rstServiceName);
void LocaleService_TransferDefaultSetting();
const std::string& LocaleService_GetBasePath();
const std::string& LocaleService_GetMapPath();
const std::string& LocaleService_GetQuestPath();

extern int is_twobyte_euckr(const char * str);

#endif

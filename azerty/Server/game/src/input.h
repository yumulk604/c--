		void		TargetInfoLoad(LPCHARACTER ch, const char* c_pData);
		void		ScriptSelectItem(LPCHARACTER ch, const void* c_pData);
		void		RealtimeSubscribe(LPCHARACTER ch, const char* c_pData);
		void		RealtimeUnsubscribe(LPCHARACTER ch, const char* c_pData);

	protected:
		void		Handshake(LPDESC d, const char * c_pData);
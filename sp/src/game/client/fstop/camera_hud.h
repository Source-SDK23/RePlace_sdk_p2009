class CCameraHUD : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE(CCameraHUD, vgui::Panel);

public:
	CCameraHUD(const char* pElementName);

	void Init();
	void MsgFunc_ShowHud(bf_read& msg);

protected:
	virtual void ApplySchemeSettings(vgui::IScheme* scheme);
	virtual void Paint(void);

private:
	bool			m_bShow;
	CHudTexture* m_pHud;
};
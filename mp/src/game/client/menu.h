//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_MENU_H
#define HUD_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#define MENU_SELECTION_TIMEOUT	5.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudMenu : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudMenu, vgui::Panel );
public:
	CHudMenu( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset( void );
	virtual bool ShouldDraw( void );
	void MsgFunc_ShowMenu( bf_read &msg );
	void HideMenu( void );
	void ShowMenu( const char * menuName, int keySlot );
	void ShowMenu_KeyValueItems( KeyValues *pKV );

	bool IsMenuOpen( void );
	void SelectMenuItem( int menu_item );

private:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
private:
	void		ProcessText( void );

	void PaintString( const wchar_t *text, int textlen, vgui::HFont& font, int x, int y );

	struct ProcessedLine
	{
		int	menuitem; // -1 for just text
		int startchar;
		int length;
		int pixels;
		int height;
	};

	CUtlVector< ProcessedLine >	m_Processed;

	int				m_nMaxPixels;
	int				m_nHeight;

	bool			m_bMenuDisplayed;
	int				m_bitsValidSlots;
	float			m_flShutoffTime;
	float			m_flExpireTime;
	int				m_fWaitingForMore;
	int				m_nSelectedItem;
	bool			m_bMenuTakesInput;

	float			m_flSelectionTime;

	bool			m_bIsFromMenuMan;

	CPanelAnimationVar( float, m_flOpenCloseTime, "OpenCloseTime", "1" );

	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255.0" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255.0" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hItemFont, "ItemFont", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hItemFontPulsing, "ItemFontPulsing", "Default" );

	CPanelAnimationVar( Color, m_MenuColor, "MenuColor", "255 255 255 255" );
	CPanelAnimationVar( Color, m_ItemColor, "ItemColor", "200 200 200 255" );
	CPanelAnimationVar( Color, m_BoxColor, "MenuBoxColor", "0 0 0 25" );
};

#endif // HUD_MENU_H

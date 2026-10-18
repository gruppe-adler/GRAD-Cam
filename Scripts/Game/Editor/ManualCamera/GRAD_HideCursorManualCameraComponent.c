//! @ingroup ManualCamera

//! Hides the editor cursor while the camera is being rotated
[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
class GRAD_HideCursorManualCameraComponent : SCR_BaseManualCameraComponent
{
	//! Mouse rotation input arrives only every other frame, so keep the cursor hidden for a moment after the last input (s)
	protected const float HOLD_S = 0.15;

	protected SCR_CursorEditorUIComponent m_Cursor;
	protected float m_fSinceRotation = HOLD_S;
	protected bool m_bHidden;
	protected int m_iDebugFrames;

	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		m_iDebugFrames++;
		if (!m_Cursor && !FindCursor())
		{
			if (m_iDebugFrames % 300 == 1)
				Print("[GRAD][Cursor] editor cursor component not found", LogLevel.NORMAL);

			return;
		}

		bool rotating;
		if (param.isManualInputEnabled)
		{
			InputManager input = GetInputManager();
			if (input.GetActionValue("ManualCameraRotateYaw") != 0 || input.GetActionValue("ManualCameraRotatePitch") != 0)
				m_fSinceRotation = 0;
		}

		m_fSinceRotation += param.timeSlice;
		rotating = m_fSinceRotation < HOLD_S;

		if (rotating != m_bHidden)
		{
			Print(string.Format("[GRAD][Cursor] rotating=%1 (manual input enabled: %2)", rotating, param.isManualInputEnabled), LogLevel.NORMAL);
			SetHidden(rotating);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool FindCursor()
	{
		SCR_MenuEditorComponent menuManager = SCR_MenuEditorComponent.Cast(SCR_MenuEditorComponent.GetInstance(SCR_MenuEditorComponent, true));
		if (!menuManager)
			return false;

		EditorMenuBase menu = menuManager.GetMenu();
		if (!menu)
			return false;

		MenuRootComponent root = menu.GetRootComponent();
		if (!root)
			return false;

		m_Cursor = SCR_CursorEditorUIComponent.Cast(root.FindComponent(SCR_CursorEditorUIComponent, true));
		if (m_Cursor)
			Print(string.Format("[GRAD][Cursor] editor cursor component found: %1", m_Cursor), LogLevel.NORMAL);

		return m_Cursor != null;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetHidden(bool hidden)
	{
		m_bHidden = hidden;

		float alpha = 1;
		if (hidden)
			alpha = 0;

		if (m_Cursor)
			m_Cursor.SetCursorAlpha(alpha);
	}

	//------------------------------------------------------------------------------------------------
	override bool EOnCameraInit()
	{
		Print("[GRAD][Cursor] hide-cursor component initialised", LogLevel.NORMAL);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnCameraExit()
	{
		if (m_bHidden)
			SetHidden(false);
	}
}

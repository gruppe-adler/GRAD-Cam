//! Hides the mouse pointer while the editor camera is being rotated.
//! Driven from SCR_CameraEditorComponent.EOnFrame (any instance), so it needs no camera prefab entry.
//!
//! Enfusion has no ShowCursor(); vanilla hides the real pointer by selecting a cursor visual with no
//! pixels (SCR_CursorCustom.REAL_CURSOR_HIDE) and restores it with cursor index 0.
//! Rotation is read from the camera parameter flags (bit 2, measured). Action values are mouse deltas
//! and read zero on frames without movement, which would make the pointer flicker.
class GRAD_CursorHider
{
	//! EManualCameraFlag bit set while the camera is rotating
	protected static const int ROTATE_BIT = 2;

	//! Cursor index restored when rotation ends (what vanilla SCR_CursorCustom restores to)
	protected static const int CURSOR_RESTORE = 0;

	protected static ref GRAD_CursorHider s_Instance;

	protected bool m_bHidden;

	//------------------------------------------------------------------------------------------------
	static GRAD_CursorHider GetInstance()
	{
		if (!s_Instance)
			s_Instance = new GRAD_CursorHider();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	void Frame()
	{
		if (!SCR_EditorManagerEntity.IsOpenedInstance())
		{
			Reset();
			return;
		}

		bool rotating;
		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (camera)
		{
			SCR_ManualCameraParam param = camera.GetCameraParam();
			rotating = param && (param.flag & ROTATE_BIT) != 0;
		}

		//! While hiding, set it every frame: the editor cursor component rewrites the cursor on state changes
		if (rotating)
		{
			WidgetManager.SetCursor(SCR_CursorCustom.REAL_CURSOR_HIDE);

			if (!m_bHidden)
				Print("[GRAD][Cursor] hidden", LogLevel.NORMAL);

			m_bHidden = true;
		}
		else if (m_bHidden)
		{
			Reset();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Never leave the pointer hidden: called when rotation ends and when the editor closes
	void Reset()
	{
		if (!m_bHidden)
			return;

		m_bHidden = false;
		WidgetManager.SetCursor(CURSOR_RESTORE);
		Print("[GRAD][Cursor] restored", LogLevel.NORMAL);
	}
}

//------------------------------------------------------------------------------------------------
//! Drives the cursor hider from every editor mode that owns a camera (Game Master, PHOTO, ...)
modded class SCR_CameraEditorComponent : SCR_BaseEditorComponent
{
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		GRAD_CursorHider.GetInstance().Frame();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnEditorClose()
	{
		super.EOnEditorClose();

		GRAD_CursorHider.GetInstance().Reset();
	}
}

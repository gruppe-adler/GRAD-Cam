//! Local-only world marker showing where the ear was dropped. Only ever created on the editor user's machine.
class GRAD_EarMarker
{
	//! Texture imported by Workbench from UI/Textures/GRAD_EarIcon.png
	protected const ResourceName ICON = "{7EE739078BF33383}UI/Textures/GRAD_EarIcon.edds";
	protected const float ICON_SIZE = 48;
	protected const string FALLBACK_TEXT = "(( EAR ))";

	protected Widget m_Marker;

	//------------------------------------------------------------------------------------------------
	protected Widget CreateMarker()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();

		//! Icon first; if the texture cannot be loaded, fall back to plain text
		ImageWidget image = ImageWidget.Cast(workspace.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NOFOCUS | WidgetFlags.IGNORE_CURSOR | WidgetFlags.STRETCH | WidgetFlags.BLEND, Color.White, 0));
		if (image && image.LoadImageTexture(0, ICON))
		{
			FrameSlot.SetSize(image, ICON_SIZE, ICON_SIZE);
			return image;
		}

		if (image)
			image.RemoveFromHierarchy();

		TextWidget text = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NOFOCUS | WidgetFlags.IGNORE_CURSOR, Color.White, 0));
		text.SetText(FALLBACK_TEXT);
		text.SetExactFontSize(22);
		FrameSlot.SetSize(text, 200, 30);
		return text;
	}

	//------------------------------------------------------------------------------------------------
	//! Marker follows the world position while visible, centred on it
	void Update(vector worldPosition, bool visible)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		vector screen;
		if (visible)
			screen = workspace.ProjWorldToScreen(worldPosition, GetGame().GetWorld());

		//! z < 0: behind the camera
		if (!visible || screen[2] < 0)
		{
			if (m_Marker)
				m_Marker.SetVisible(false);
			return;
		}

		if (!m_Marker)
		{
			m_Marker = CreateMarker();
			Print(string.Format("[GRAD][Ear] marker created: %1 at screen %2", m_Marker, screen), LogLevel.NORMAL);
		}

		vector size = FrameSlot.GetSize(m_Marker);
		m_Marker.SetVisible(true);
		FrameSlot.SetPos(m_Marker, screen[0] - size[0] * 0.5, screen[1] - size[1] * 0.5);
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		if (m_Marker)
			m_Marker.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void ~GRAD_EarMarker()
	{
		if (m_Marker)
			m_Marker.RemoveFromHierarchy();
	}
}

//------------------------------------------------------------------------------------------------
//! Editor user's side of the ear: input, camera streaming, dropping and the marker.
//! A singleton on purpose: several SCR_CameraEditorComponent instances exist (one per editor mode)
//! and open/close events do not reliably reach all of them, so any instance may drive Frame().
class GRAD_EarClient
{
	//! Our own action (R key), defined in the addon's chimeraInputCommon.conf override
	protected static const string ACTION_TOGGLE = "GRAD_EarToggle";
	protected static const string CONTEXT_EAR = "GRAD_EarContext";
	protected static const float STREAM_MIN_MOVE_SQ = 0.25; //!< 0.5 m
	protected static const float STREAM_HEARTBEAT_MS = 1000;
	protected static const float DROP_RANGE = 1000;
	protected static const float HINT_DURATION_S = 5;

	protected static ref GRAD_EarClient s_Instance;

	protected ref GRAD_EarMarker m_Marker = new GRAD_EarMarker();
	protected bool m_bActive;
	protected bool m_bHasDrop;
	protected vector m_vDropPosition;
	protected IEntity m_DropTarget;
	protected vector m_vDropOffset;
	protected vector m_vLastStreamed;
	protected float m_fLastStream_ms;

	//------------------------------------------------------------------------------------------------
	static GRAD_EarClient GetInstance()
	{
		if (!s_Instance)
			s_Instance = new GRAD_EarClient();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Called every frame by whichever editor camera component is running. Safe to call more than once per frame.
	void Frame()
	{
		//! Game Master / admin only: limited editors get no ear controls (the server refuses them too)
		if (!SCR_EditorManagerEntity.IsOpenedInstance() || SCR_EditorManagerEntity.IsLimitedInstance())
		{
			Deactivate();
			return;
		}

		if (!m_bActive)
			Activate();

		GetGame().GetInputManager().ActivateContext(CONTEXT_EAR);

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		//! The marker is part of the editor interface: it hides together with it (toggled with I)
		bool dropped = m_bHasDrop && controller.GRAD_GetEarMode() == GRAD_EEarMode.DROPPED && IsInterfaceVisible();
		m_Marker.Update(GetDropPosition(), dropped);

		if (controller.GRAD_GetEarMode() == GRAD_EEarMode.CAMERA)
			StreamCamera(controller);
	}

	//------------------------------------------------------------------------------------------------
	//! False while the user has hidden the editor interface
	protected bool IsInterfaceVisible()
	{
		SCR_MenuEditorComponent menuManager = SCR_MenuEditorComponent.Cast(SCR_MenuEditorComponent.GetInstance(SCR_MenuEditorComponent, true));
		return !menuManager || menuManager.IsVisible();
	}

	//------------------------------------------------------------------------------------------------
	void Deactivate()
	{
		if (!m_bActive)
			return;

		m_bActive = false;
		GetGame().GetInputManager().RemoveActionListener(ACTION_TOGGLE, EActionTrigger.DOWN, OnToggle);
		m_Marker.Hide();
	}

	//------------------------------------------------------------------------------------------------
	protected void Activate()
	{
		m_bActive = true;
		GetGame().GetInputManager().AddActionListener(ACTION_TOGGLE, EActionTrigger.DOWN, OnToggle);
		Print("[GRAD][Ear] activated", LogLevel.NORMAL);

		//! The server ignores us while the editor is closed, so state is sent again on every opening
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		if (controller.GRAD_GetEarMode() == GRAD_EEarMode.DROPPED && m_bHasDrop)
			controller.GRAD_SetEar(GRAD_EEarMode.DROPPED, GetDropPosition(), m_DropTarget);
		else
			controller.GRAD_SetEar(GRAD_EEarMode.CAMERA, vector.Zero);
	}

	//------------------------------------------------------------------------------------------------
	//! Dropped point follows its target when one is attached
	protected vector GetDropPosition()
	{
		if (m_DropTarget)
			return m_DropTarget.CoordToParent(m_vDropOffset);

		return m_vDropPosition;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnToggle(float value, EActionTrigger reason)
	{
		if (!m_bActive)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		if (controller.GRAD_GetEarMode() == GRAD_EEarMode.DROPPED)
		{
			controller.GRAD_SetEar(GRAD_EEarMode.CAMERA, vector.Zero);
			SCR_HintManagerComponent.ShowCustomHint("You now hear players around the camera. Press R to drop the ear at the crosshair.", "Ear: camera", HINT_DURATION_S);
		}
		else
		{
			Drop(controller);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Drop(SCR_PlayerController controller)
	{
		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (!camera)
			return;

		vector mat[4];
		camera.GetWorldTransform(mat);

		TraceParam trace = new TraceParam();
		trace.Start = mat[3];
		trace.End = mat[3] + mat[2] * DROP_RANGE;
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.Exclude = camera;

		float fraction = GetGame().GetWorld().TraceMove(trace, null);
		vector hit = trace.Start + (trace.End - trace.Start) * fraction;

		//! Attach to the character/vehicle that was hit, so the ear travels with it
		IEntity target;
		if (trace.TraceEnt)
		{
			IEntity root = trace.TraceEnt.GetRootParent();
			if (ChimeraCharacter.Cast(root) || Vehicle.Cast(root))
				target = root;
		}

		m_bHasDrop = true;
		m_DropTarget = target;
		m_vDropPosition = hit;
		if (target)
			m_vDropOffset = target.CoordToLocal(hit);

		controller.GRAD_SetEar(GRAD_EEarMode.DROPPED, hit, target);

		if (target)
			SCR_HintManagerComponent.ShowCustomHint("The ear is attached and follows it. Press R to return it to the camera.", "Ear: attached", HINT_DURATION_S);
		else
			SCR_HintManagerComponent.ShowCustomHint("The ear stays at this point. Press R to return it to the camera.", "Ear: dropped", HINT_DURATION_S);
	}

	//------------------------------------------------------------------------------------------------
	//! Camera mode: report movement, with a slow heartbeat to recover from lost unreliable packets
	protected void StreamCamera(SCR_PlayerController controller)
	{
		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (!camera)
			return;

		vector mat[4];
		camera.GetWorldTransform(mat);

		float now_ms = GetGame().GetWorld().GetWorldTime();
		bool moved = vector.DistanceSq(mat[3], m_vLastStreamed) > STREAM_MIN_MOVE_SQ;
		if (!moved && now_ms - m_fLastStream_ms < STREAM_HEARTBEAT_MS)
			return;

		m_vLastStreamed = mat[3];
		m_fLastStream_ms = now_ms;
		controller.GRAD_StreamCameraPosition(mat[3]);
	}
}

//------------------------------------------------------------------------------------------------
//! Drives the ear client from every editor mode that owns a camera (Game Master, PHOTO, ...)
modded class SCR_CameraEditorComponent : SCR_BaseEditorComponent
{
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		GRAD_EarClient.GetInstance().Frame();
		GRAD_CursorHider.GetInstance().Frame();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnEditorClose()
	{
		super.EOnEditorClose();

		GRAD_EarClient.GetInstance().Deactivate();
		GRAD_CursorHider.GetInstance().Reset();
	}
}

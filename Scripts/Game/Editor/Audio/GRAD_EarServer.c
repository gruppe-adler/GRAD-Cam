//! Where the editor user's "ears" are placed
enum GRAD_EEarMode
{
	CAMERA, //!< Follow the editor camera
	DROPPED //!< Fixed point in the world, or attached to an entity
}

//------------------------------------------------------------------------------------------------
//! Ear state of one player. Lives on the player controller, so it exists on the owner and on the
//! server and disappears together with the player.
modded class SCR_PlayerController : PlayerController
{
	protected GRAD_EEarMode m_eGradEarMode;
	protected vector m_vGradEarPosition;
	protected RplId m_GradEarTargetId = RplId.Invalid();
	protected vector m_vGradEarOffset; //!< Position in the target's local space

	//------------------------------------------------------------------------------------------------
	GRAD_EEarMode GRAD_GetEarMode()
	{
		return m_eGradEarMode;
	}

	//------------------------------------------------------------------------------------------------
	//! Owner side: change the ear placement. For DROPPED, target may be null (free point).
	void GRAD_SetEar(GRAD_EEarMode mode, vector worldPosition, IEntity target = null)
	{
		RplId targetId = RplId.Invalid();
		vector offset;

		if (mode == GRAD_EEarMode.DROPPED && target)
		{
			RplComponent rpl = RplComponent.Cast(target.FindComponent(RplComponent));
			if (rpl)
			{
				targetId = rpl.Id();
				offset = target.CoordToLocal(worldPosition);
			}
		}

		ApplyEar(mode, worldPosition, targetId, offset);

		if (Replication.IsServer())
			return;

		Rpc(GRAD_RpcEar, mode, worldPosition, targetId, offset);
	}

	//------------------------------------------------------------------------------------------------
	//! Owner side: camera mode movement. Unreliable, losing a packet is harmless.
	void GRAD_StreamCameraPosition(vector worldPosition)
	{
		m_vGradEarPosition = worldPosition;

		if (!Replication.IsServer())
			Rpc(GRAD_RpcEarMove, worldPosition);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyEar(GRAD_EEarMode mode, vector worldPosition, RplId targetId, vector offset)
	{
		m_eGradEarMode = mode;
		m_vGradEarPosition = worldPosition;
		m_GradEarTargetId = targetId;
		m_vGradEarOffset = offset;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GRAD_RpcEar(GRAD_EEarMode mode, vector worldPosition, RplId targetId, vector offset)
	{
		if (!GRAD_IsEditorOpen())
			return;

		ApplyEar(mode, worldPosition, targetId, offset);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void GRAD_RpcEarMove(vector worldPosition)
	{
		if (m_eGradEarMode != GRAD_EEarMode.CAMERA || !GRAD_IsEditorOpen())
			return;

		m_vGradEarPosition = worldPosition;
	}

	//------------------------------------------------------------------------------------------------
	//! Server side: only Game Masters / admins (unlimited editor) with the editor opened can place ears.
	//! Limited editors, e.g. players with only a restricted photo mode, are refused.
	protected bool GRAD_IsEditorOpen()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity manager = core.GetEditorManager(GetPlayerId());
		return manager && manager.IsOpened() && !manager.IsLimited();
	}

	//------------------------------------------------------------------------------------------------
	//! Server side: current ear position. False when vanilla behaviour should apply.
	bool GRAD_GetEarPosition(out vector position)
	{
		if (!GRAD_IsEditorOpen())
			return false;

		if (m_eGradEarMode == GRAD_EEarMode.DROPPED && m_GradEarTargetId != RplId.Invalid())
		{
			RplComponent rpl = RplComponent.Cast(Replication.FindItem(m_GradEarTargetId));
			IEntity target;
			if (rpl)
				target = rpl.GetEntity();

			//! Attached entity is gone: stay where it was last seen
			if (target)
				m_vGradEarPosition = target.CoordToParent(m_vGradEarOffset);
			else
				m_GradEarTargetId = RplId.Invalid();
		}

		position = m_vGradEarPosition;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! The engine asks the script where an editor user hears from
modded class SCR_VoNComponent : VoNComponent
{
	override protected event vector GetEditorWorldLocation(int playerId)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		vector position;
		if (controller && controller.GRAD_GetEarPosition(position))
			return position;

		return super.GetEditorWorldLocation(playerId);
	}
}

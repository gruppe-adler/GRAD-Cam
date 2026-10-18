//! Where the editor user's "ears" are placed
enum GRAD_EEarMode
{
	CAMERA, //!< Follow the editor camera
	DROPPED //!< Fixed point in the world, or attached to an entity (the owner streams the moving position)
}

//------------------------------------------------------------------------------------------------
//! Server-side diagnostics: at most one line per key every few seconds
class GRAD_EarDebug
{
	protected static ref map<string, float> s_mLast = new map<string, float>();

	static bool Ready(string key, float interval_ms = 3000)
	{
		float now_ms = GetGame().GetWorld().GetWorldTime();
		float last;
		if (s_mLast.Find(key, last) && now_ms - last < interval_ms)
			return false;

		s_mLast.Set(key, now_ms);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Ear state of one player. Lives on the player controller, so it exists on the owner and on the
//! server and disappears together with the player.
modded class SCR_PlayerController : PlayerController
{
	protected GRAD_EEarMode m_eGradEarMode;
	protected vector m_vGradEarPosition;
	protected int m_iGradRejected;

	//------------------------------------------------------------------------------------------------
	GRAD_EEarMode GRAD_GetEarMode()
	{
		return m_eGradEarMode;
	}

	//------------------------------------------------------------------------------------------------
	//! Owner side: change the ear placement. Only the position matters to the server and it reaches the
	//! server through GRAD_StreamPosition, so no separate message is sent here.
	void GRAD_SetEar(GRAD_EEarMode mode, vector worldPosition)
	{
		m_eGradEarMode = mode;
		m_vGradEarPosition = worldPosition;
	}

	//------------------------------------------------------------------------------------------------
	//! Owner side: the ear position, sent whenever it moves and as a slow heartbeat. Unreliable,
	//! losing a packet is harmless because the next one follows. Used for every ear mode.
	void GRAD_StreamPosition(vector worldPosition)
	{
		m_vGradEarPosition = worldPosition;

		if (!Replication.IsServer())
			Rpc(GRAD_RpcEarMove, worldPosition);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void GRAD_RpcEarMove(vector worldPosition)
	{
		if (!GRAD_IsAllowed())
			return;

		m_vGradEarPosition = worldPosition;

		//! Vanilla treats the editor manager entity's position as the editor user's position (GetEditorWorldLocation
		//! and the engine's editor lookups read it), so keep that entity on the ear even if the client's own
		//! replicated movement of it does not arrive
		SCR_EditorManagerEntity manager = GRAD_GetManager();
		if (manager)
		{
			vector mat[4];
			manager.GetWorldTransform(mat);
			mat[3] = worldPosition;
			manager.SetWorldTransform(mat);
		}

		if (GRAD_EarDebug.Ready("rx" + GetPlayerId().ToString()))
		{
			vector managerPos;
			if (manager)
			{
				vector check[4];
				manager.GetWorldTransform(check);
				managerPos = check[3];
			}

			Print(string.Format("[GRAD][Ear] received position %1 from player %2, manager entity now at %3", worldPosition, GetPlayerId(), managerPos), LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! This player's editor manager entity, null when none exists
	protected SCR_EditorManagerEntity GRAD_GetManager()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return null;

		return core.GetEditorManager(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	//! Server side: only Game Masters / admins (unlimited editor) with the editor opened can place ears.
	//! Limited editors, e.g. players with only a restricted photo mode, are refused.
	protected bool GRAD_IsAllowed()
	{
		SCR_EditorManagerEntity manager = GRAD_GetManager();

		bool allowed = manager && manager.IsOpened() && !manager.IsLimited();

		//! Diagnostics: report the first few refusals so a wrongly rejected mode (e.g. photo) shows up in the server log
		if (!allowed && m_iGradRejected < 10)
		{
			m_iGradRejected++;

			bool opened, limited;
			if (manager)
			{
				opened = manager.IsOpened();
				limited = manager.IsLimited();
			}

			Print(string.Format("[GRAD][Ear] refused player %1: manager=%2 opened=%3 limited=%4", GetPlayerId(), manager != null, opened, limited), LogLevel.WARNING);
		}

		return allowed;
	}

	//------------------------------------------------------------------------------------------------
	//! Server side: current ear position. False when vanilla behaviour should apply.
	bool GRAD_GetEarPosition(out vector position)
	{
		if (!GRAD_IsAllowed())
			return false;

		position = m_vGradEarPosition;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! The engine asks the script where an editor user hears from
modded class SCR_VoNComponent : VoNComponent
{
	//! Client side: a voice was delivered to this machine
	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{
		super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);

		if (GetGame().GetPlayerController())
			GRAD_EarClient.LogVoice(playerId, isSenderEditor);
	}

	override protected event vector GetEditorWorldLocation(int playerId)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		vector position;
		if (controller && controller.GRAD_GetEarPosition(position))
		{
			if (GRAD_EarDebug.Ready("poll" + playerId.ToString()))
				Print(string.Format("[GRAD][Ear] engine asked for editor location of player %1, answered with %2", playerId, position), LogLevel.NORMAL);

			return position;
		}

		vector fallback = super.GetEditorWorldLocation(playerId);
		if (GRAD_EarDebug.Ready("fallback" + playerId.ToString()))
			Print(string.Format("[GRAD][Ear] engine asked for editor location of player %1, using vanilla %2 (controller=%3)", playerId, fallback, controller != null), LogLevel.NORMAL);

		return fallback;
	}
}

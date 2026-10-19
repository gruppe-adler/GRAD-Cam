//! Clean speech for the editor user.
//! Vanilla hands the player's voice to the editor manager while the editor is open, which sounds like a
//! degraded radio. The speaking voice is put back on whatever the player is controlling (their character,
//! or the AI they took over), so everybody hears them normally, from that body.
class GRAD_EditorVoice
{
	protected static const int RESTORE_DELAY_MS = 300;

	//! Voice the player had when the editor was requested to open
	protected static SCR_VoNComponent s_PreviousVoice;

	//------------------------------------------------------------------------------------------------
	//! Voice component of the controlled entity, as long as it is not the editor manager's own
	static SCR_VoNComponent FindBodyVoice(SCR_EditorManagerEntity manager)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return null;

		SCR_VONController von = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		if (von)
		{
			SCR_VoNComponent current = von.GetVONComponent();
			if (current && current != manager.FindComponent(SCR_VoNComponent))
				return current;
		}

		IEntity body = pc.GetControlledEntity();
		if (!body)
			return null;

		return SCR_VoNComponent.Cast(body.FindComponent(SCR_VoNComponent));
	}

	//------------------------------------------------------------------------------------------------
	static void Remember(SCR_EditorManagerEntity manager)
	{
		s_PreviousVoice = FindBodyVoice(manager);
	}

	//------------------------------------------------------------------------------------------------
	//! Delayed, because vanilla changes the voice component as part of opening
	static void ScheduleRestore(SCR_EditorManagerEntity manager)
	{
		ScriptCallQueue queue = GetGame().GetCallqueue();
		queue.Remove(Restore);
		queue.CallLater(Restore, RESTORE_DELAY_MS, false, manager);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Restore(SCR_EditorManagerEntity manager)
	{
		SCR_VoNComponent voice = s_PreviousVoice;
		s_PreviousVoice = null;

		if (!voice && manager)
			voice = FindBodyVoice(manager);

		PlayerController pc = GetGame().GetPlayerController();
		if (!voice || !pc)
			return;

		SCR_VONController von = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		if (von)
			von.SetVONComponent(voice);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_EditorManagerEntity
{
	override void Open(bool showErrorNotification = true)
	{
		bool opening = !m_bIsOpened;

		//! Server: keep vanilla from attaching an editor radio to the player's VON controller
		if (opening && IsAuthority() && !FindComponent(BaseRadioComponent))
		{
			PlayerController owner = GetGame().GetPlayerManager().GetPlayerController(GetPlayerID());
			SCR_VONController ownerVon;
			if (owner)
				ownerVon = SCR_VONController.Cast(owner.FindComponent(SCR_VONController));

			if (ownerVon)
				ownerVon.m_bIsEditorRadioAdded = true;
		}

		if (opening && IsOwner())
			GRAD_EditorVoice.Remember(this);

		super.Open(showErrorNotification);

		if (IsOwner())
			GRAD_EditorVoice.ScheduleRestore(this);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_VONController
{
	//! Transmitting without a voice component is pointless; try to get one first
	override protected void SetActiveTransmit(notnull SCR_VONEntry entry)
	{
		if (!m_VONComp)
			AssignVONComponent();

		if (m_VONComp)
			super.SetActiveTransmit(entry);
	}
}

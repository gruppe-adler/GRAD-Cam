[ComponentEditorProps(category: "GRAD", description: "Controls movement and rotation inertia strength in sync via keybind and ArmaVision slider")]
class GRAD_InertiaControlCameraComponentClass : SCR_ManualCameraBaseComponentClass {}

class GRAD_InertiaControlCameraComponent : SCR_ManualCameraBaseComponent
{
	[Attribute("1", UIWidgets.Slider, "Inertia Strength", "0 2 0.1")]
	protected float m_fInertiaStrength;

	protected SCR_MovementInertiaManualCameraComponent m_MovementInertia;
	protected SCR_RotationInertiaManualCameraComponent m_RotationInertia;

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		m_MovementInertia = SCR_MovementInertiaManualCameraComponent.Cast(
			owner.FindComponent(SCR_MovementInertiaManualCameraComponent));
		m_RotationInertia = SCR_RotationInertiaManualCameraComponent.Cast(
			owner.FindComponent(SCR_RotationInertiaManualCameraComponent));

		SyncInertiaComponents();

		GetGame().GetInputManager().AddActionListener("GRAD_InertiaIncrease", EActionTrigger.DOWN, OnInertiaIncrease);
		GetGame().GetInputManager().AddActionListener("GRAD_InertiaDecrease", EActionTrigger.DOWN, OnInertiaDecrease);
	}

	protected void OnInertiaIncrease()
	{
		SetInertiaStrength(m_fInertiaStrength + 0.1);
		NotifySettingsUI();
	}

	protected void OnInertiaDecrease()
	{
		SetInertiaStrength(m_fInertiaStrength - 0.1);
		NotifySettingsUI();
	}

	protected void NotifySettingsUI()
	{
		SCR_ManualCameraSettingsEditorUIComponent settingsUI = SCR_ManualCameraSettingsEditorUIComponent.Cast(
			SCR_EditorManagerEntity.GetInstance().FindComponent(SCR_ManualCameraSettingsEditorUIComponent));
		if (settingsUI)
			settingsUI.UpdateSliderFromComponent();
	}

	void SetInertiaStrength(float value)
	{
		m_fInertiaStrength = Math.Clamp(Math.Round(value * 10) * 0.1, 0.0, 2.0);
		SyncInertiaComponents();
	}

	float GetInertiaStrength()
	{
		return m_fInertiaStrength;
	}

	protected void SyncInertiaComponents()
	{
		if (m_MovementInertia)
			m_MovementInertia.m_fInertiaStrength = m_fInertiaStrength;
		if (m_RotationInertia)
			m_RotationInertia.m_fInertiaStrength = m_fInertiaStrength;
	}
}

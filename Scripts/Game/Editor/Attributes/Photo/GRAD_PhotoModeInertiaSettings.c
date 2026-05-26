// TODO: Confirm the exact base class name in Workbench — search for the component
// that owns the FOV slider in the ArmaVision settings panel and use that class here.
modded class SCR_ManualCameraSettingsEditorUIComponent : SCR_BaseEditorUIComponent
{
	protected SliderWidget m_InertiaSlider;
	protected GRAD_InertiaControlCameraComponent m_InertiaComponent;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_InertiaSlider = SliderWidget.Cast(w.FindAnyWidget("InertiaSlider"));
		if (!m_InertiaSlider)
			return;

		m_InertiaComponent = FindInertiaComponent();
		if (!m_InertiaComponent)
			return;

		m_InertiaSlider.SetMin(0.0);
		m_InertiaSlider.SetMax(2.0);
		m_InertiaSlider.SetStep(0.1);
		m_InertiaSlider.SetCurrent(m_InertiaComponent.GetInertiaStrength());
		m_InertiaSlider.m_OnChanged.Insert(OnInertiaSliderChanged);
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_InertiaSlider)
			m_InertiaSlider.m_OnChanged.Remove(OnInertiaSliderChanged);

		super.HandlerDeattached(w);
	}

	protected void OnInertiaSliderChanged(Widget w, float value)
	{
		if (m_InertiaComponent)
			m_InertiaComponent.SetInertiaStrength(value);
	}

	// Called by keybind component to keep slider in sync when value changes via keys
	void UpdateSliderFromComponent()
	{
		if (m_InertiaSlider && m_InertiaComponent)
			m_InertiaSlider.SetCurrent(m_InertiaComponent.GetInertiaStrength());
	}

	protected GRAD_InertiaControlCameraComponent FindInertiaComponent()
	{
		SCR_ManualCameraEditorComponent cameraManager = SCR_ManualCameraEditorComponent.Cast(
			SCR_EditorManagerEntity.GetInstance().FindComponent(SCR_ManualCameraEditorComponent));
		if (!cameraManager)
			return null;

		IEntity camera = cameraManager.GetCamera();
		if (!camera)
			return null;

		return GRAD_InertiaControlCameraComponent.Cast(
			camera.FindComponent(GRAD_InertiaControlCameraComponent));
	}
}

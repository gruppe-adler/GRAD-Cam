modded class SCR_MenuOverlaysEditorComponent : SCR_BaseEditorComponent
{
	override void EOnEditorPostActivate()
	{
		super.EOnEditorPostActivate();

		//! Index 0 is "none" on these layers
		SetLayerIndex(EEditorMenuOverlayLayer.VIGNETTE, 0);
		SetLayerIndex(EEditorMenuOverlayLayer.HELPER, 0);
		SetLayerIndex(EEditorMenuOverlayLayer.LOGO_FRAME, 0);

		//! Index 0 is a real entry here (a logo / name tags); "none" is index 1
		SetLayerIndex(EEditorMenuOverlayLayer.LOGO, 1);
		SetLayerIndex(EEditorMenuOverlayLayer.PLAYER, 1);
	}

	protected void SetLayerIndex(EEditorMenuOverlayLayer layerType, int index)
	{
		SCR_EditorMenuOverlayLayer layer = GetOverlayLayer(layerType);
		if (!layer)
			return;

		array<SCR_EditorMenuOverlay> overlays = {};
		if (layer.GetOverlays(overlays) <= index)
			return;

		//! Forced: the layer may already sit on this index while its widget still shows
		layer.SetCurrentOverlay(index, true);
	}
}

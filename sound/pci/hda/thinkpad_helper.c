/* Helper functions for Thinkpad LED control;
 * to be included from codec driver
 */

#if IS_ENABLED(CONFIG_THINKPAD_ACPI) || IS_ENABLED(CONFIG_HID_LENOVO)

#include <linux/acpi.h>
#include <linux/hid-lenovo.h>
#include <linux/thinkpad_acpi.h>

static int (*led_set_func)(int, bool);
static void (*old_vmaster_hook)(void *, int);
static uint8_t tpacpi = 1;

static bool is_thinkpad(struct hda_codec *codec)
{
	return (codec->core.subsystem_id >> 16 == 0x17aa);
}

static bool is_thinkpad_acpi(struct hda_codec *codec)
{
	return (codec->core.subsystem_id >> 16 == 0x17aa) &&
	       (acpi_dev_found("LEN0068") || acpi_dev_found("IBM0068"));
}

static void update_thinkpad_mute_led(void *private_data, int enabled)
{
	if (old_vmaster_hook)
		old_vmaster_hook(private_data, enabled);

	if (led_set_func)
		led_set_func(tpacpi ? TPACPI_LED_MUTE : HID_LENOVO_LED_MUTE, !enabled);
}

static void update_thinkpad_micmute_led(struct hda_codec *codec,
				      struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (!ucontrol || !led_set_func)
		return;
	if (strcmp("Capture Switch", ucontrol->id.name) == 0 && ucontrol->id.index == 0) {
		/* TODO: How do I verify if it's a mono or stereo here? */
		bool val = ucontrol->value.integer.value[0] || ucontrol->value.integer.value[1];
		led_set_func(tpacpi ? TPACPI_LED_MICMUTE : HID_LENOVO_LED_MICMUTE, !val);
	}
}

static int hda_fixup_thinkpad_acpi(struct hda_codec *codec)
{
	struct hda_gen_spec *spec = codec->spec;
	int ret = -ENXIO;

	if (!is_thinkpad(codec))
		return -ENODEV;
	if (!is_thinkpad_acpi(codec))
		return -ENODEV;
	if (!led_set_func)
		led_set_func = symbol_request(tpacpi_led_set);
	if (!led_set_func) {
		codec_warn(codec,
			   "Failed to find thinkpad-acpi symbol tpacpi_led_set\n");
		return -ENOENT;
	}

	tpacpi = 1;

	if (led_set_func(TPACPI_LED_MUTE, false) >= 0) {
		old_vmaster_hook = spec->vmaster_mute.hook;
		spec->vmaster_mute.hook = update_thinkpad_mute_led;
		ret = 0;
	}

	if (led_set_func(TPACPI_LED_MICMUTE, false) >= 0) {
		if (spec->num_adc_nids > 1)
			codec_dbg(codec,
				  "Skipping micmute LED control due to several ADCs");
		else {
			spec->cap_sync_hook = update_thinkpad_micmute_led;
			ret = 0;
		}
	}

	return ret;
}

static int hda_fixup_thinkpad_hid(struct hda_codec *codec)
{
	struct hda_gen_spec *spec = codec->spec;
	int ret = 0;

	if (!is_thinkpad(codec))
		return -ENODEV;
	if (!led_set_func)
		led_set_func = symbol_request(hid_lenovo_led_set);
	if (!led_set_func) {
		codec_warn(codec,
			   "Failed to find hid-lenovo symbol hid_lenovo_led_set\n");
		return -ENOENT;
	}

	tpacpi = 0;

	// do not remove hook if setting delay does not work currently
	// it is a usb hid devices which is maybe connected later
	led_set_func(HID_LENOVO_LED_MUTE, false);
	old_vmaster_hook = spec->vmaster_mute.hook;
	spec->vmaster_mute.hook = update_thinkpad_mute_led;

	led_set_func(HID_LENOVO_LED_MICMUTE, false);
	spec->cap_sync_hook = update_thinkpad_micmute_led;

	return ret;
}

static void hda_fixup_thinkpad(struct hda_codec *codec,
				    const struct hda_fixup *fix, int action)
{
	int remove = 0;

	if (action == HDA_FIXUP_ACT_PROBE) {
		if (hda_fixup_thinkpad_acpi(codec))
			remove = hda_fixup_thinkpad_hid(codec);
	}


	if (led_set_func && (action == HDA_FIXUP_ACT_FREE || remove)) {
		if (tpacpi)
			symbol_put(tpacpi_led_set);
		else
			symbol_put(hid_lenovo_led_set);

		led_set_func = NULL;
		old_vmaster_hook = NULL;
	}
}

#else /* CONFIG_THINKPAD_ACPI */

static void hda_fixup_thinkpad(struct hda_codec *codec,
				    const struct hda_fixup *fix, int action)
{
}

#endif /* CONFIG_THINKPAD_ACPI */

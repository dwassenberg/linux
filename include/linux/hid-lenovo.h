
#ifndef __HID_LENOVO_H__
#define __HID_LENOVO_H__


enum {
	HID_LENOVO_LED_MUTE,
	HID_LENOVO_LED_MICMUTE,
	HID_LENOVO_LED_FNLOCK,
	HID_LENOVO_LED_MAX,
};

int hid_lenovo_led_set(int led_num, bool on);

#endif /* __HID_LENOVO_H_ */

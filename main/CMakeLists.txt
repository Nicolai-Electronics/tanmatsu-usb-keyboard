idf_build_get_property(idf_target IDF_TARGET)

idf_component_register(
	SRCS
		"main.c"
		"usb_mode.c"
		"usb_keyboard.c"
	PRIV_REQUIRES
		esp_lcd
		fatfs
		nvs_flash
		badge-bsp
		esp_tinyusb
	INCLUDE_DIRS
		"."
)

idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-variable" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-const-variable" APPEND)

SET(SOURCES screens / ui_mainScreen.c
    screens / ui_GamePadAppScreen.c
    screens / ui_RcAppScreen.c
    screens / ui_GamePadAppSettingScreen.c
    ui.c
    ui_helpers.c
    images / ui_img_game_pad_app_png.c
    images / ui_img_nes_app_png.c
    images / ui_img_rc_app_png.c
    images / ui_img_setting_app_png.c
    images / ui_img_ble_icon_20px_png.c
    images / ui_img_wifi_icon_18px_png.c
    images / ui_img_usb_icon_20px_png.c
    images / ui_img_return_btn_16px_png.c
    images / ui_img_setting_icon_20px_png.c
    images / ui_img_up_btn_png.c
    images / ui_img_left_btn_png.c
    images / ui_img_right_btn_png.c
    images / ui_img_down_btn_png.c
    images / ui_img_espnow_bind_png.c
    images / ui_img_ble_28_png.c
    images / ui_img_brilliance_28_png.c
    images / ui_img_usb_28_png.c
    images / ui_img_vibration_28_png.c
    images / ui_img_volume_ctrl_26_png.c
    images / ui_img_wifi_28_png.c
    fonts / ui_font_SourceHanSansCN14.c
    fonts / ui_font_SourceHanSansCN20.c)

add_library(ui $ {SOURCES})

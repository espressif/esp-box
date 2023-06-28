SET(SOURCES screens/ui_ScreenSetup.c
    screens/ui_ScreenWifiReset.c
    screens/ui_ScreenListen.c
    screens/ui_ScreenSettings.c
    screens/ui_ScreenReset.c
    ui.c
    ui_helpers.c
    images/ui_img_setup_bg_png.c
    images/ui_img_setup_text_bg_png.c
    images/ui_img_reset_icon_png.c
    images/ui_img_settings_back_png.c
    images/ui_img_body_shadow_png.c
    images/ui_img_body_png.c
    images/ui_img_body_eye_screen_png.c
    images/ui_img_sleep_body_eyes_png.c
    images/ui_img_sleep_big_z_png.c
    images/ui_img_sleep_small_z_png.c
    images/ui_img_listen_back_glow_png.c
    images/ui_img_listen_body_eyes_1_png.c
    images/ui_img_reply_chatgpt_bg_png.c
    images/ui_img_reply_body_png.c
    images/ui_img_reply_small_shadow_png.c
    images/ui_img_reply_chatgpt_logo_png.c
    images/ui_img_settings_icon_png.c
    images/ui_img_listen_body_eyes_2_png.c
    fonts/ui_font_KaiTiCN20.c
    fonts/ui_font_PingFangEN14.c
    fonts/ui_font_PingFangEN16.c
    fonts/ui_font_PingFangEN20.c)

add_library(ui ${SOURCES})

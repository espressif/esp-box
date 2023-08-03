SET(SOURCES screens/ui_ScreenSetup.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_setup_bg_png.c
    images/ui_img_setup_text_bg_png.c
    fonts/ui_font_PingFangEN14.c
    fonts/ui_font_PingFangEN16.c
    fonts/ui_font_PingFangEN20.c)

add_library(ui ${SOURCES})

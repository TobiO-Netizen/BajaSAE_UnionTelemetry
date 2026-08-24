SET(SOURCES ui_ScreenBaja.c
    ui_ScreenStandard.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_gaugebajanotext_png.c
    ui_img_pointer_png.c
    ui_img_transmissiontempsymbolinverted_png.c
    ui_img_image_png.c
    ui_img_rpm_symbol_inverted_png.c
    ui_img_839238605.c)

add_library(ui ${SOURCES})

SET(SOURCES ui_HomeScreen.c
    ui_FindAppScreen.c
    ui_LightAppScreen.c
    ui_SoundManagerScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_lightapp_png.c
    ui_img_findappicon_png.c
    ui_img_1280652718.c
    ui_img_lightoff3_png.c
    ui_img_lighton3_png.c
    ui_img_ellipse_22_png.c
    ui_img_stop_png.c
    ui_img_resume_png.c)

add_library(ui ${SOURCES})

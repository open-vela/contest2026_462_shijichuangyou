# Contest 2026 team 000 hello app
if(CONFIG_LVX_USE_DEMO_CONTEST2026_000_HELLO_APP)
  nuttx_add_application(
    NAME hello_app
    SRCS hello_app_main.c
    STACKSIZE 2048)
endif()

if(CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE)
  nuttx_add_application(
    NAME ebadge
    STACKSIZE ${CONFIG_CONTEST2026_462_EBADGE_STACKSIZE}
    MODULE ${CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE}
    DEPENDS lvgl
    SRCS ebadge_main.c core/ebadge_controller.c core/ebadge_store.c
         core/ebadge_calendar.c core/ebadge_record.c core/ebadge_power.c
         core/ebadge_shake.c core/ebadge_imu.c
         ui/ebadge_theme.c ui/ebadge_ornament.c ui/ebadge_deco.c
         ui/ebadge_display.c
         ui/ebadge_view.c ui/ebadge_pages.c ui/ebadge_date_editor.c
         ui/ebadge_text_editor.c ui/ebadge_portraits.c ui/ebadge_motion.c
         ui/ebadge_font_zh_20.c ui/ebadge_font_title_26.c)
endif()

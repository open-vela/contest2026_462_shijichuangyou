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
    SRCS ebadge_main.c core/ebadge_controller.c ui/ebadge_view.c
         ui/ebadge_font_zh_20.c)
endif()

############################################################################
# Contest 2026 team 000 hello app - Makefile
############################################################################

include $(APPDIR)/Make.defs

PROGNAME =
PRIORITY =
STACKSIZE =
MODULE =
MAINSRC =

ifneq ($(CONFIG_LVX_USE_DEMO_CONTEST2026_000_HELLO_APP),)
PROGNAME += hello_app
PRIORITY += SCHED_PRIORITY_DEFAULT
STACKSIZE += 2048
MODULE += $(CONFIG_LVX_USE_DEMO_CONTEST2026_000_HELLO_APP)
MAINSRC += hello_app_main.c
endif

ifneq ($(CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE),)
PROGNAME += ebadge
PRIORITY += SCHED_PRIORITY_DEFAULT
STACKSIZE += $(CONFIG_CONTEST2026_462_EBADGE_STACKSIZE)
MODULE += $(CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE)
MAINSRC += ebadge_main.c
CSRCS += core/ebadge_controller.c ui/ebadge_view.c ui/ebadge_font_zh_20.c ui/ebadge_pages.c ui/ebadge_date_editor.c ui/ebadge_text_editor.c ui/ebadge_portraits.c ui/ebadge_motion.c
endif

include $(APPDIR)/Application.mk

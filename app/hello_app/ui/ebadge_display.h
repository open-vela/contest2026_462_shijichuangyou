/* SPDX-License-Identifier: Apache-2.0 */
/* Panel power adapter.
 *
 * L2 of the idle policy has to actually remove panel power; a dark overlay is
 * explicitly not accepted as evidence for it. The only route openvela exposes
 * here is the LCD character driver's power ioctl:
 *
 *   drivers/lcd/lcd_dev.c   case LCDDEVIO_SETPOWER -> lcd_ptr->setpower()
 *   vendor/sifli/.../lcd/sf32lb_lcd.c  sf32lb_lcd_setpower() -> DisplayOn/Off
 *
 * The device name comes from that driver (it registers "/dev/lcd%i"), not from
 * guesswork. This panel is AMOLED and the vendor tree ships no backlight
 * device, so there is no brightness to scale: power is on or off. When the
 * device cannot be opened the call fails loudly instead of pretending.
 */
#ifndef EBADGE_DISPLAY_H
#define EBADGE_DISPLAY_H
#include <stdbool.h>

/* Remove panel power. False when the display device could not be driven. */
bool ebadge_display_off(void);
/* Restore panel power at full level. */
bool ebadge_display_on(void);
/* True when the panel power ioctl has actually succeeded at least once. */
bool ebadge_display_verified(void);
/* One-line status for the acceptance screen. Never NULL. */
const char *ebadge_display_status(void);

#endif

# On-device anniversary editor

Tap the anniversary page to open a modal year/month/day editor. Each field
has plus/minus touch buttons. Years are bounded to 2024..2099, months to 1..12,
and days to the selected Gregorian month's length. Changing year or month
clamps an invalid day, including leap-year transitions.

Confirm updates the anniversary and countdown immediately. Cancel discards
the draft. Without a saved date, the initial draft uses device local date if
it is in range, otherwise 2026-01-01. This is explicitly labeled a draft and
is never saved until the user confirms. The dialog blocks underlying page
gestures while open and is released with the application's view.

All controls inherit the selected DIY palette and Chinese font. No persistent
write is performed: the dialog explicitly says the date lasts for this run.
System-clock calibration is still separate; selecting an anniversary does not
set RTC or make an invalid system clock valid.

Compilation and physical touch checks must be reported separately. No hardware
acceptance is claimed by this document. Existing invalid-time font fallback
and root LVGL event warnings remain open; other reference features remain in
REFERENCE_FEATURES.md and HARDWARE_FEATURE_EVIDENCE.md.

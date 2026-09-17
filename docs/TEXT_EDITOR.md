# On-device support-text editor

Tap the support-text page to edit the current message. The modal editor
provides a text area, standard Latin/numeric keyboard, and three Chinese phrase
insertion buttons (companionship, support, birthday wishes). The original text
is preserved until confirmation; cancel discards the draft. Confirmation updates
the rolling text immediately and keeps the existing palette.

Input must contain non-whitespace content and fit 159 UTF-8 bytes plus a NUL
terminator. Oversize confirmation stays in the editor with a Chinese error
message instead of silently truncating the saved message. The text area also
has a 159-character input limit. Settings remain RAM-only as displayed in the UI.

This is NOT a full Chinese IME: Chinese phrase insertion plus Latin/numeric
editing is the currently implemented input path. The embedded font covers the
UI and bundled phrases, not arbitrary Chinese Unicode. A full character font,
Chinese input or a real phone content-transfer path, persistence, and physical
keyboard/gesture acceptance remain open requirements.

The modal overlay blocks gestures reaching the underlying page. Confirm/cancel
keyboard events and the separate Chinese buttons share the same handlers.
The input is detached from the keyboard before the overlay is destroyed.

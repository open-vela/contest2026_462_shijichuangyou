#!/bin/sh
# Build and run the host-side tests for the badge's pure logic.
#
# These cover the on-card settings record codec and the anniversary calendar
# arithmetic. Neither needs LVGL or a board, so a wrong answer can be caught
# here instead of on the panel.
#
# Usage: tools/run_logic_test.sh
set -e
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

binary=${TMPDIR:-/tmp}/ebadge_logic_test

# Deliberately not the cross compiler: this is host verification.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -O1 \
  -Iapp/hello_app/core -Iapp/hello_app/ui \
  -o "$binary" \
  tools/test_logic.c \
  app/hello_app/core/ebadge_calendar.c \
  app/hello_app/core/ebadge_record.c

exec "$binary"

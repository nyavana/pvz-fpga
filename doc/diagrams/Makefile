# Makefile — render every D2 source in this catalog to .svg + .png.
#
# Targets:
#   all                render every diagram (default)
#   clean              remove generated .svg / .png; leave .d2 alone
#   01-system          render only Section 1 diagrams
#   02-hw              render only Section 2 diagrams
#   03-sw              render only Section 3 diagrams
#   04-interface       render only Section 4 diagrams
#   05-cross-cutting   render only Section 5 diagrams
#   06-flow            render only Section 6 diagrams
#
# common.d2 is import-only and is intentionally excluded from rendering.

SECTIONS := 01-system 02-hw 03-sw 04-interface 05-cross-cutting 06-flow

# Find every .d2 under section folders (excludes common.d2 by construction).
D2_SOURCES := $(foreach s,$(SECTIONS),$(wildcard $(s)/*.d2))
SVG_TARGETS := $(D2_SOURCES:.d2=.svg)
PNG_TARGETS := $(D2_SOURCES:.d2=.png)

# d2 0.7.1 renders elk by default; pin it here for reproducibility.
# Fall back to dagre if elk emits a warning (D2 returns 0 but prints to
# stderr; the wrapper catches that case).
D2 ?= d2
D2_LAYOUT ?= elk

.PHONY: all clean check-d2 $(SECTIONS)

all: check-d2 $(SVG_TARGETS) $(PNG_TARGETS)

# Per-section convenience targets render only that section's diagrams.
define section_rule
$(1): check-d2 $$(patsubst %.d2,%.svg,$$(wildcard $(1)/*.d2)) \
              $$(patsubst %.d2,%.png,$$(wildcard $(1)/*.d2))
endef
$(foreach s,$(SECTIONS),$(eval $(call section_rule,$(s))))

# .svg / .png each depend on the .d2 source AND common.d2 so editing
# the shared classes file forces a rebuild.
%.svg: %.d2 common.d2
	@echo "  D2  $< -> $@"
	@$(D2) --layout=$(D2_LAYOUT) $< $@ 2> $@.stderr; \
	rc=$$?; \
	if [ $$rc -ne 0 ] || grep -q -i 'warn' $@.stderr; then \
	  if [ $$rc -ne 0 ]; then \
	    echo "  elk failed for $<, retrying with dagre..."; \
	  else \
	    echo "  elk warned for $<, retrying with dagre..."; \
	  fi; \
	  $(D2) --layout=dagre $< $@ || { rm -f $@.stderr; exit 1; }; \
	fi; \
	rm -f $@.stderr

%.png: %.d2 common.d2
	@echo "  D2  $< -> $@"
	@$(D2) --layout=$(D2_LAYOUT) $< $@ 2> $@.stderr; \
	rc=$$?; \
	if [ $$rc -ne 0 ] || grep -q -i 'warn' $@.stderr; then \
	  if [ $$rc -ne 0 ]; then \
	    echo "  elk failed for $<, retrying with dagre..."; \
	  else \
	    echo "  elk warned for $<, retrying with dagre..."; \
	  fi; \
	  $(D2) --layout=dagre $< $@ || { rm -f $@.stderr; exit 1; }; \
	fi; \
	rm -f $@.stderr

# Check that d2 is on PATH before doing anything; emit a useful error
# if not. Sentinel target so we only check once per `make` invocation.
check-d2:
	@command -v $(D2) >/dev/null 2>&1 || { \
	  echo "error: '$(D2)' not found on PATH."; \
	  echo "       Install D2 v0.7.1+ from https://d2lang.com/tour/install"; \
	  echo "       (the renders committed to git let you read the catalog"; \
	  echo "        without it, but rebuilding requires d2.)"; \
	  exit 127; \
	}

clean:
	@find $(SECTIONS) -type f \( -name '*.svg' -o -name '*.png' -o -name '*.stderr' \) -print -delete

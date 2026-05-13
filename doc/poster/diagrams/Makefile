D2 ?= d2

D2_FILES := $(filter-out _%.d2,$(wildcard *.d2))
SVG_FILES := $(D2_FILES:.d2=.svg)
PNG_FILES := $(D2_FILES:.d2=.png)

.PHONY: all svg png clean
all: svg

svg: $(SVG_FILES)
png: $(PNG_FILES)

%.svg: %.d2 _style.d2
	$(D2) $< $@

%.png: %.d2 _style.d2
	$(D2) $< $@

clean:
	rm -f $(SVG_FILES) $(PNG_FILES)

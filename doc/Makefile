
HTMLFLAGS=--no-split --css-ref="./texinfo-style.css"

DOCNAME=sl-manual
TEXI=$(addsuffix .texi, $(DOCNAME))
PDF=$(addsuffix .pdf, $(DOCNAME))
HTML=$(addsuffix .html, $(DOCNAME))

.PHONY: all pdf html clean

all: pdf html
pdf: $(PDF)
html: $(HTML)

clean:
	rm -f $(PDF) $(HTML)
	rm -f *.aux *.fn *.fns *.ky *.log *.vr *.vrs *.toc

#-------------------------------------------------------------------------------

$(PDF): $(TEXI)
	makeinfo --pdf -o $@ $^

$(HTML): $(TEXI)
	makeinfo --html -o $@ $(HTMLFLAGS) $^

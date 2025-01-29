
HTMLFLAGS=--no-split --css-ref="./texinfo-style.css"

DOCNAME=sl-manual.org
TEXI=$(DOCNAME:.org=.texi)
PDF=$(DOCNAME:.org=.pdf)
HTML=$(DOCNAME:.org=.html)

.PHONY: all pdf html clean

all: pdf html
pdf: $(PDF)
html: $(HTML)

clean:
	rm -f $(TEXI) $(PDF) $(HTML)
	rm -f *.aux *.fn *.fns *.ky *.log *.vr *.vrs *.toc

#-------------------------------------------------------------------------------

%.texi: %.org
	emacs --batch --quick --load="org" --file="$<" --funcall="org-texinfo-export-to-texinfo"

%.pdf: %.texi
	makeinfo --pdf -o $@ $<

%.html: %.texi
	makeinfo --html -o $@ $(HTMLFLAGS) $<

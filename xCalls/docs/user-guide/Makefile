# ----------------------------------------------------------------------------
#			  Simple latex makefile
#
#		   Robert Grimm, rgrimm@cs.washington.edu
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
#  Tools
# ----------------------------------------------------------------------------

LATEX	     = latex
PDFLATEX     = pdflatex
BIBTEX	     = bibtex --min-crossrefs=1000
MV	     = mv
RM	     = rm -f

export BIBINPUTS = /p/multifacet/lib/bibtex/
#export BIBINPUTS = ~/research/LogTM/papers/bibtex

# ----------------------------------------------------------------------------
#  Source file (without extension)
# ----------------------------------------------------------------------------

SOURCE	     = main


# ----------------------------------------------------------------------------
#  Rules
# ----------------------------------------------------------------------------

.PHONY: simple pdf ps dvi all clean

default: pdf

#pdf: $(SOURCE).tex $(SOURCE).bbl
pdf: $(SOURCE).tex
	$(PDFLATEX) $(SOURCE)
	$(PDFLATEX) $(SOURCE)

ps: $(SOURCE).dvi
	$(DVIPS) $(SOURCE)

dvi: $(SOURCE).dvi

ispell:
	ispell -p ./ispell.dict *.tex

all: pdf ps

$(SOURCE).dvi: $(SOURCE).tex $(SOURCE).bbl
	$(LATEX) $(SOURCE)
	$(LATEX) $(SOURCE)

$(SOURCE).bbl: $(SOURCE).tex 
	$(PDFLATEX) $(SOURCE)
	$(BIBTEX) $(SOURCE)
	perl -n fixbbl.pl < $(SOURCE).bbl > tmp.bbl
	$(MV) tmp.bbl $(SOURCE).bbl

clean:
	$(RM) $(SOURCE).aux
	$(RM) $(SOURCE).bbl
	$(RM) $(SOURCE).blg
	$(RM) $(SOURCE).dvi
	$(RM) $(SOURCE).log
	$(RM) $(SOURCE).out
	$(RM) $(SOURCE).pdf
	$(RM) $(SOURCE).ps
	$(RM) $(SOURCE).bak

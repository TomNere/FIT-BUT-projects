all: doc

doc: proj1.tex
	latex proj1.tex
	latex proj1.tex
	dvips -t a4 proj1.dvi
	ps2pdf proj1.ps

clean:
	rm -f *.dvi *.log *.aux *.ps *.pdf
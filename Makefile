CC = gcc
CFLAG = -Wall

run: showFDtables
	./showFDtables

perprocess: showFDtables
	./showFDtables --per-process

systemWide: showFDtables
	./showFDtables --systemWide

Vnodes: showFDtables
	./showFDtables --Vnodes

composite: showFDtables
	./showFDtables --composite

output_TXT: showFDtables
	./showFDtables --output_TXT

output_binary: showFDtables
	./showFDtables --output_binary

showFDtables:
	$(CC) $(CFLAG) System-Wide-FD.c -o $@

clean:
	rm -f showFDtables *.txt *.bin


.PHONY : help
help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  run 		 : RunshowFDtables"
	@echo "  perprocess      : Run showFDtables with --per-process "
	@echo "  systemWide      : Run showFDtables with --systemWide"
	@echo "  Vnodes    	 : Run showFDtables with --Vnodes"
	@echo "  composite       : Run showFDtables with --composite"
	@echo "  output_TXT      : Run showFDtables with --output_TXT"
	@echo "  output_binary   : Run showFDtables with --output_binary"
	@echo "  clean           : Remove generated files"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "ir.h"

struct specialdata {
	uint16_t offset;
	uint16_t section;
}__attribute__((packed));

struct section {
	unsigned start;
	unsigned long *labels;
	
	uint16_t *code;
};

struct script {
	uint32_t numsections;
	uint32_t *sectionoffsets;
	
	unsigned numspecial;
	struct specialdata *specials;
	
	struct section *sections;
};

int main(int argc, char **argv) {
	/* idea:
	- open
	- parse line by line, store labels with their addresses, store references to labels as-if
	- patch up all labels
	- emit
	- close
	*/
	FILE *f, *o;
	unsigned insize, gamenum;
	char *input = NULL;
	struct ir_script *script = NULL;
	
	if(argc != 4) {
		printf("usage: %s gamenum infile outfile\n", argv[0]);
		return 1;
	}
	
	gamenum = strtoul(argv[1], NULL, 10) - 1;
	
	if(!(f = fopen(argv[2], "r"))) {
		printf("couldnt open %s as input\n", argv[2]);
		return 1;
	}
	
	fseek(f, 0, SEEK_END);
	insize = ftell(f);
	fseek(f, 0, SEEK_SET);
	input = malloc(insize+1);
	fread(input, insize, 1, f);
	input[insize] = 0;
	fclose(f);
	
	lexer_init(input);
	if(!lexer_scan()) printf("couldnt lex file\n");
	
	if(!(script = parser_parse(gamenum))) return 1;
	
	ir_script_dump(script);
	
	if(!ir_script_preprocess(script, gamenum)) return 1;
	
	ir_script_dump(script);
	/* for fixing up addresses for sections and labels */
	if(!ir_script_fixup(script, gamenum)) return 1;
	
	ir_script_dump(script);
	
	
	if(!(o = fopen(argv[3], "wb"))) {
		printf("couldnt open %s as output\n", argv[3]);
		return 1;
	}
	
	ir_script_emit(o, script);
	
	ir_script_free(script);
	
	lexer_finalize();
	
	free(input);
	fclose(o);
}
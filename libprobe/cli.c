#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "cli.h"

////////////////
/// CLI CODE ///

cli_parser* new_cli(char *buffer,
			 int size,
			 exec_command_function_pointer exec_command,
			 void *opaque){
    cli_parser* ret = (cli_parser*) malloc(sizeof(cli_parser));
    if (ret == NULL){
	fprintf(stderr, "NÃ£o foi possÃ­vel alocar memÃ³ria\n");
	_exit(1);
    }
    ret->state = STATE_NULL;
    ret->buffer = buffer;
    ret->buffer_size = size;
    ret->exec_command = exec_command;
    ret->opaque = opaque;
#ifdef _MSC_VER
    ret->interactive = 1;
#else // _MSC_VER
    ret->interactive = isatty(STDIN_FILENO);
#endif // _MSC_VER
    return ret;
}

char *regen_line(int argc, const char **argv)
{
    // + 1 copia o \0 final
    size_t size = (size_t)(argv[argc - 1] - argv[0] + (long)strlen(argv[argc - 1]) + 1);
    if (size > 1024) {
	fputs("Cara, que linha grande hein\nnÃ£o tava esperando\n", stderr);
	_exit(1);
    }
    char *ret = (char *) malloc(size);
    if (ret == NULL){
	fputs("NÃ£o foi possÃ­vel alocar memÃ³ria\n", stderr);
	_exit(1);
    }
    memcpy(ret, argv[0], size);
    for (int i=1; i < argc; i++){
	ret[argv[i] - 1 - argv[0]] = ' ';
    }

    return ret;
}

int process_line(cli_parser* data, char* input_buf, int len){
    int status = 0;
    for (char* p = input_buf; status == 0 && p < input_buf + len; p++){
	status = process_byte(data, *p);
    }
    if (status == 0){
	status = process_byte(data, '\n');
    }
    return status;
}

int process_byte(cli_parser* data, char input)
{
    if(data->state == STATE_NULL){
	data->buffer[0] = 0;
	data->argc = 1;
	data->argv[0] = data->buffer;
	data->state = STATE_PARSING_LINE;
	data->cursor = NULL;
    } else if (data->state == STATE_STOP){
	fprintf(stderr, "O interpretador de comandos estÃ¡ sendo invocado depois de ter "
		"sido terminado.\n");
	return -1;
    } else if (data->state != STATE_PARSING_LINE){
	fprintf(stderr, "Estado %d invÃ¡lido.\n", data->state);
	return -1;
    }

    if (data->cursor == 0){
	data->cursor = data->buffer;
    } else if (data->cursor - data->buffer >= data->buffer_size - 1){
	fprintf(stderr, "O buffer de leitura do cli_parser estÃ¡ cheio, nada a fazer\n");
	fprintf(stderr, "%d %d %d\n", data->cursor, data->cursor - data->buffer, data->buffer_size - 1);
	return -1;
    }
    *data->cursor++ = input;

    // debug: MantÃ©m o buffer sempre terminado com 0
    // A linha Ã© inÃ³cua ao resto do programa
    // nÃ£o fazer isso a cada byte Ã© uma otimizaÃ§Ã£o
    // data->cursor[1] = 0;

    if (input == 24 || input == 4){
	// Ctrl-X (cancel) ou Ctrl-D (end of transmission)
	data->state = STATE_STOP;
	return 0;
    } else if (input == 127){
	// Backspace (DEL)
	data->cursor--;
	if (data->cursor >= data->buffer){
	    *data->cursor = 0;
	    data->cursor--;
	}
	if (data->cursor < data->buffer){
	    data->cursor = data->buffer;
	}
    } else if (input == ' ' ||
		input == '\n'){
	*(data->cursor-1) = 0;
	if (input == ' '){
	    data->argv[data->argc] = data->cursor;
	    data->argc++;
	} else {
	    int ret;
	    const char *const_argv[sizeof(data->argv)];
	    char **p=data->argv;
	    const char **c=const_argv;
	    while (p < data->argv + data->argc){
		*c++ = *p++;
	    }
	    if (RET_QUIT == (ret = data->exec_command(data->opaque,
						   data->argc,
						   const_argv))){
		data->state = STATE_STOP;
	    }
	    data->cursor = NULL;
	    data->argc = 1;
	    if (data->state == STATE_PARSING_LINE){
		prompt(data);
	    }
	    return ret;
	}
    }
    return 0;
}

void prompt(cli_parser* data){
    if (data->interactive){
	printf(">");
	fflush(stdout);
    }
}

int check_arg_count(int argc, const char **argv, int required)
{
    if (argc - 1 < required){
	if (required == 1){
	    fprintf(stderr, "Comando \"%s\" requer pelo menos 1 argumento.\n", argv[0]);
	} else {
	    fprintf(stderr, "Comando \"%s\" requer pelo menos %d argumentos.\n", argv[0], required);
	}
	return -1;
    }
    return 0;

}

/* Some key codes
^V22
^V22
^V22
^J^J10
10
^A1
^X24
^X24
^V22
^B2
^J10
*/


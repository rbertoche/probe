#ifndef CLI_H
#define CLI_H
/////////////
// CLI API //

// Para obter comportamento de terminal na sua aplicaÃ§Ã£o, basta implementar o
// comportamento do terminal em exec_command, criar um cli_parser com esse
// exec_command, e em seguida passar a entrada atravÃ©s de process_byte ou
// process_line.


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// Estados do interpretador
typedef enum {
    STATE_NULL = 0,
    STATE_PARSING_LINE,
    STATE_STOP
} cli_parser_state;

typedef enum {
    RET_QUIT = -10000
} exec_command_ret;

// Ponteiro de funÃ§Ã£o que permite determinar o comportamento do cli_parser
typedef int (*exec_command_function_pointer)(void *opaque, int argc, const char **argv);

// cli_parser struct
typedef struct cli_parser_struct {
    cli_parser_state state;
    char *buffer;
    char *argv[50];
    int buffer_size;
    char *cursor;
    int argc;
    int interactive;
    void *opaque;
    exec_command_function_pointer exec_command;
} cli_parser;

// Recebe dados byte a byte da aplicaÃ§Ã£o
int process_byte(cli_parser* data, char input);

// Recebe dados linha a linha da aplicaÃ§Ã£o
int process_line(cli_parser* data, char* input_buf, int len);

// Ao passar a stream de uma fonte para o cli_parser atravÃ©s de uma das duas
// funÃ§Ãµes process_byte e process_line, as linhas passadas serÃ£o interpretadas
// como linhas de comando e entregues a exec_command.

// Instancia a abstraÃ§Ã£o de cli_parser representada atravÃ©s
// de um ponteiro para sua struct
// O comportamento da instÃ¢ncia de cli_parser depende do
// argumento exec_command, que deve ser implementado pela
// aplicaÃ§Ã£o
// Chame free nesse ponteiro apÃ³s o uso do cli_parser
cli_parser* new_cli(char *buffer,
			 int size,
			 exec_command_function_pointer exec_command,
			 void *opaque);

// Exibe um prompt antes da espera por comando do usuÃ¡rio
void prompt(cli_parser* data);

// Ãštil para reconstruir a linha de comando inputada, jÃ¡ que todos
// os espaÃ§os sÃ£o trocados por \0 no buffer durante o parsing
char *regen_line(int argc, const char **argv);

// Testa se nÃºmero de argumentos Ã© suficicente e gera na saÃ­da de erro
// uma mensagem se for o caso
int check_arg_count(int argc, const char **argv, int required);

/* TODO:
  permitir que a aplicaÃ§Ã£o passe um espaÃ§o de memÃ³ria para ser repassado
  a exec_command
*/

#ifdef __cplusplus
}
#endif // __cplusplus

/////////////
#endif // CLI_H



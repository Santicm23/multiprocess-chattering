
/*
  Ejecución:
  ./talker -i ID -p pipenom
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "structures.h"

#define max_msg 100

void validate_args(int argc, char *argv[]);
int count_words(char *str);
Request create_req(int ID, char *str);
int empty(char *str);
void delete_req(Request req);
void send_request(Request req);
void auth();
void list(Request req);
void group(Request req);
void sent(Request req);
void salir(Request req);
void signalHandler();

int ID;
char pipeNom[25], pipeUnit[8];

int main(int argc, char *argv[]) {
  
  char prompt[200];

  validate_args(argc, argv);

  // mkfifo(pipeNom, 0666);
  sprintf(pipeUnit, "pipe%d", ID);//casting

  auth();

  signal(SIGUSR1, &signalHandler);

  do {
    Request req;

    printf("$ ");

    if (fgets(prompt, 200, stdin) == 0) {
      perror("\nError en la entrada de datos\n\n");
      exit(EXIT_FAILURE);
    }

    // eliminar \n del string
    prompt[strcspn(prompt, "\n")] = 0;

    if (empty(prompt))
      continue;

    req = create_req(ID, prompt);

    if (strcmp(req.type, "Salir") == 0) {
      salir(req);
      printf("\nDesconectando del sistema\n\n");

    } else if (strcmp(req.type, "List") == 0) {
      list(req);

    } else if (strcmp(req.type, "Group") == 0) {
      group(req);

    } else if (strcmp(req.type, "Sent") == 0) {
      sent(req);

    } else {
      printf("El comando no existe\n");
    }

  } while (strcmp(prompt, "Salir"));

  // unlink(pipeNom);

  return 0;
}

void validate_args(int argc, char *argv[]) {
  if (argc != 5) {
    perror("El manager requiere solo dos banderas (-i y -p con sus respectivos "
           "valores)");
    exit(1);
  } else {
    for (int i = 1; i < argc; i += 2) {
      if (strcmp(argv[i], "-i") == 0) {
        ID = atoi(argv[i + 1]);

      } else if (strcmp(argv[i], "-p") == 0) {

        strcpy(pipeNom, argv[i + 1]);

      } else {
        printf("\nLa flag '%s' no es válida\n\n", argv[i]);
        exit(1);
      }
    }
  }
}

void send_request(Request req) {
  int fd = open(pipeNom, O_WRONLY);

  if (fd == -1) {
    perror("Error de apertura del FIFO");
    exit(EXIT_FAILURE);
  }

  if (write(fd, &req, sizeof(req)) == 0) {
    perror("error en la escritura");
    exit(1);
  }

  close(fd);
}

void auth() {
  Request req;

  strcpy(req.type, "Auth");

  req.ID = ID;
  req.c_args = 1;

  sprintf(req.args, "%d", getpid());

  send_request(req);

  char msg[50];

  int fd_tmp = open("auth", O_RDONLY);

  if (read(fd_tmp, &msg, sizeof(msg)) == -1) {
    perror("Error lectura");
    exit(EXIT_FAILURE);
  }

  close(fd_tmp);

  printf("\n%s\n\n", msg);
  
  if (strcmp("Error", strtok(msg, ":")) == 0) {
    exit(EXIT_FAILURE);
  }

  mode_t fifo_mode = S_IRUSR | S_IWUSR;
  mkfifo(pipeUnit, fifo_mode);
}

void list(Request req) {
  char recibo[100];
  send_request(req);
  
  int fd_tmp = open(pipeUnit, O_RDONLY);

  if (read(fd_tmp, &recibo, sizeof(recibo)) == -1) {
    perror("Error lectura");
    exit(EXIT_FAILURE);
  }

  printf("Los usuarios conectados en el sistema son %s\n", recibo);
  close(fd_tmp);
}

void group(Request req) { send_request(req); }

void sent(Request req) { send_request(req); }

void salir(Request req) {
  send_request(req);
  unlink(pipeUnit);
}

int empty(char *str) {
  int k = 0;
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] == ' ')
      k++;
  }
  return k == strlen(str);
}

int count_words(char *str) {
  int k = 1;

  for (int i = 0; str[i] != '\0'; i++) {
    if (i > 0 && str[i - 1] != ' ' && str[i] == ' ')
      k++;
  }
  return k;
}

Request create_req(int ID, char *str) {
  Request req;
  char *token;
  int c_words = count_words(str);

  req.ID = ID;
  req.c_args = c_words - 1; // sin el tipo

  token = strtok(str, " ");
  strcpy(req.type, token);

  if (req.c_args > 0) {
    token = strtok(NULL, "\0");
    strcpy(req.args, token);
  }

  return req;
}

void signalHandler() {
  char recibo[150];
  
  int fd_tmp = open(pipeUnit, O_RDONLY);

  if (read(fd_tmp, &recibo, sizeof(recibo)) == -1) {
    perror("Error lectura");
    exit(EXIT_FAILURE);
  }

  close(fd_tmp);
  
  printf("\n--> %s\n", recibo);
}
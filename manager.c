
/*
  Ejecución:
  ./Manager -n N -p pipenom
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <regex.h>

#include "structures.h"


#define max_res 200


void validate_args(int argc, char *argv[]);
void auth(Request req);
void list(Request req);
void list_group(Request req);
void sent(Request req);
void group(Request req);
char status_t(int ID);
int pidTalker(int ID);
void sent_group(Request req);
int match(char *regex, char *str);

int N;
char pipeNom[25];

Talker *talkers;
Group *groups;

int c_talkers = 0, c_talkers_conectados = 0, c_groups = 0;

int main(int argc, char *argv[]) {

  validate_args(argc, argv);

  printf("\nManager iniciado con un número máximo de %d usuarios y '%s' como "
         "nombre del pipe pricipal \n\n",
         N, pipeNom);
  
  talkers = malloc(0);
  groups = malloc(0);

  mode_t fifo_mode = S_IRUSR | S_IWUSR;

  mkfifo("auth", fifo_mode);
  mkfifo(pipeNom, fifo_mode);

  do {
    int fd = open(pipeNom, O_RDONLY);

    if (fd == -1) {
      perror("Error de apertura del FIFO");
      exit(EXIT_FAILURE);
    }

    Request req;

    if (read(fd, &req, sizeof(req)) == -1) {
      perror("Error lectura");
      exit(EXIT_FAILURE);
    }

    printf("\nMe llego esta info:\n\n");
    printf("ID: %d\n", req.ID);
    printf("tipo: '%s'\n", req.type);
    printf("info: '%s'\n", req.args);
    printf("cant_args: %d\n", req.c_args);

    if (strcmp(req.type, "Auth") == 0) {
      auth(req);
      
    } else if (strcmp(req.type, "List") == 0) {
      if (req.c_args == 0)
        list(req);
      else if (req.c_args == 1)
        list_group(req);
      else
        perror("solicitud inválida (List)");
        
    } else if (strcmp(req.type, "Group") == 0) {
      group(req);
      
    } else if (strcmp(req.type, "Sent") == 0) {
      char tmp[max_res];
      strcpy(tmp, req.args);
      
      char *token;
      token = strtok(tmp, "\"");
      printf("'%s'\n", token);
      token = strtok(NULL, " ");
      printf("'%s'\n", token);
      
      if (match("^([0-9]+)", token)) {
        sent(req);
      
      } else {
        sent_group(req);
      }
      
    } else if (strcmp(req.type, "Salir") == 0) {
      for (int i = 0; i < c_talkers; i++) {
        if (req.ID == talkers[i].ID){
          talkers[i].conectado = 0;
        }
      }
      c_talkers_conectados--;
      
    } else {
      perror("llego una solicitud inválida\n");
    }

    close(fd);
    
  } while (1);

  free(talkers);
  free(groups);

  unlink(pipeNom);
  unlink("auth");

  return 0;
}

void validate_args(int argc, char *argv[]) {
  if (argc != 5) {
    perror("El manager requiere solo dos banderas (-n y -p con sus respectivos "
           "valores)");
    exit(1);
  } else {
    for (int i = 1; i < argc; i += 2) {
      if (strcmp(argv[i], "-n") == 0) {
        N = atoi(argv[i + 1]);

      } else if (strcmp(argv[i], "-p") == 0) {
        strcpy(pipeNom, argv[i + 1]);

      } else {
        printf("\nLa flag '%s' no es válida\n\n", argv[i]);
        exit(1);
      }
    }
  }
}

void auth(Request req) {
  if (req.c_args != 1) {
    perror("solicitud inválida (auth)");
    
  } else if (c_talkers_conectados == N) {
    char msg[50] = "Error: Límite de usuarios, intentelo más tarde";
    printf("%s", msg);

    int fd_tmp = open("auth", O_WRONLY);

    if (write(fd_tmp, &msg, sizeof(msg)) == 0) {
      perror("error en la escritura");
      exit(1);
    }

    close(fd_tmp);
    
  } else {
    int conected = 0;
    for (int i = 0; i < c_talkers; i++) {
      if (req.ID == talkers[i].ID && talkers[i].conectado) {
        conected = 1;
        break;
      }
    }
    if (conected) {
      char msg[] = "Error: el usuario ya esta conectado";

      int fd_tmp = open("auth", O_WRONLY);

      if (write(fd_tmp, &msg, sizeof(msg)) == 0) {
        perror("error en la escritura");
        exit(1);
      }

      close(fd_tmp);
      
    } else {
      char* token = strtok(req.args, " ");
      
      talkers = realloc(talkers, (c_talkers+1) * sizeof(Talker));
      
      talkers[c_talkers].ID = req.ID;
      talkers[c_talkers].pid = atoi(token);
      talkers[c_talkers].conectado = 1;
      
      c_talkers++;
      c_talkers_conectados++;

      char msg[] = "El usuario se guardo correctamente";

      int fd_tmp = open("auth", O_WRONLY);

      if (write(fd_tmp, &msg, sizeof(msg)) == 0) {
        perror("error en la escritura");
        exit(1);
      }

      close(fd_tmp);
    }
  }
}

void list(Request req) {
  char pipe_tmp[7], aux[10], envio[max_res] = "Los usuarios conectados en el sistema son ";

  int primero = 1;
  
  for (int i = 0; i < c_talkers; i++) {
    if (talkers[i].conectado) {
      if (primero) {
        primero = 0;
      } else {
        strcat(envio, ", ");
      }
      sprintf(aux, "%d", talkers[i].ID);//casting
      strcat(envio, aux);
    }
  }

  printf("se envia a %s, la info: %s", pipe_tmp, envio);
  
  sprintf(pipe_tmp, "pipe%d", req.ID);
  int fdtemp = open(pipe_tmp, O_WRONLY);
  
  if (write(fdtemp, &envio, sizeof(envio)) == 0) {
    perror("error en la escritura");
    exit(1);
  }
  
  close(fdtemp);
}

void list_group(Request req) {
  
  int g = atoi(req.args + 1);

  char pipe_tmp[7], aux[10], envio[max_res] = "Los usuarios del grupo son ";

  int primero = 1;
  if (g < c_groups) {  
    for (int i = 0; i < groups[g].c_talkers; i++) {
      if (primero) {
        primero = 0;
      } else {
        strcat(envio, ", ");
      }
    
      sprintf(aux, "%d", groups[g].ID_talkers[i]);//casting
      strcat(envio, aux);
    }
  } else {
    strcpy(envio, "Error:");
  }
  
  printf("se envia a %s, la info: %s", pipe_tmp, envio);
  
  sprintf(pipe_tmp, "pipe%d", req.ID);
  int fdtemp = open(pipe_tmp, O_WRONLY);
  
  if (write(fdtemp, &envio, sizeof(envio)) == 0) {
    perror("error en la escritura");
    exit(1);
  }
  
  close(fdtemp);
    
}

void group(Request req) {
  int *contenedor;
  char res;
  contenedor = malloc(req.c_args * sizeof(int)); 

  contenedor[0] = atoi(strtok(req.args, ", "));
  res = status_t(contenedor[0]);
  if (res == 'n') {
    printf("El usuario %d no existe\n", contenedor[0]);
  } else {
    for(int i = 1; i < req.c_args; i++) {
      contenedor[i] = atoi(strtok(NULL, ", "));
      res = status_t(contenedor[i]);
      if (res == 'n') {
        printf("El usuario %d no existe\n", contenedor[0]);
        break;
      }
    }
    
  }
  //---------------------------------------------------------------------/
  if (res != 'n') {
    groups = realloc(groups, (c_groups + 1) * sizeof(Group));
    groups[c_groups].ID = c_groups;
    groups[c_groups].c_talkers = req.c_args;
    groups[c_groups].ID_talkers = malloc((req.c_args + 1) * sizeof(int));

    int pidtemp;
    char devolucion[max_res], pipe_tmp[7];
    sprintf(devolucion, "Se ha creado el grupo G%d con los talkers ", c_groups);

    int primero = 1;
    
    for (int i = 0; i < req.c_args; i++) {
      char mensaje[max_res];
      sprintf(mensaje, "Ahora perteneces al grupo G%d", c_groups);
      
      groups[c_groups].ID_talkers[i] = contenedor[i];
      pidtemp = pidTalker(contenedor[i]);
      
      if (kill(pidtemp, SIGUSR1)) {
        perror("Error mandando la señal");        
      } else {
        printf("\nse le notifico al talker\n\n");
      }

      char tmp[4];

      if (primero) {
        primero = 0;
      } else {
        strcat(devolucion, ", ");
      }
      
      sprintf(tmp, "%d", contenedor[i]);
      strcat(devolucion, tmp);

      sprintf(pipe_tmp, "pipe%d", contenedor[i]);
      int fd_tmp = open(pipe_tmp, O_WRONLY);
      
      if (write(fd_tmp, &mensaje, sizeof(mensaje)) == 0) {
        perror("Error en la escritura");
        exit(1);
      }
      
      close(fd_tmp);
    }
    groups[c_groups].ID_talkers[req.c_args] = req.ID;
    groups[c_groups].c_talkers++;
    char tmp[8];
    sprintf(tmp, " y %d", req.ID);
    strcat(devolucion, tmp);

    sprintf(pipe_tmp, "pipe%d", req.ID);
    int fd_tmp = open(pipe_tmp,O_WRONLY);
    if (write(fd_tmp, &devolucion, sizeof(devolucion)) == 0) {
        perror("Error en la escritura");
        exit(1);
    }
    close(fd_tmp);
    
    c_groups++;
    
  } else {
    char devolucion[max_res] = "Alguno de los IDs ingresados no esta registrado";
    char pipe_tmp[7];
    sprintf(pipe_tmp, "pipe%d", req.ID);
    int fd_tmp = open(pipe_tmp,O_WRONLY);
    if (write(fd_tmp, &devolucion, sizeof(devolucion)) == 0) {
      perror("Error en la escritura");
      exit(1);
    }
    close(fd_tmp);
  }
}

void sent(Request req) {
  int check = 0;
  char pipe_tmp[7], res[150];
  char *envio = strtok(req.args, "\"");
  char *id_dest = strtok(NULL, " ");
  
  int pidDest;
  int Ide = atoi(id_dest);
  
  for (int i = 0; i < c_talkers; i++) {
    if (Ide == talkers[i].ID && talkers[i].conectado) {
      pidDest = talkers[i].pid;
      check = 1;
      break;
    }
  }  
  if (check) {
    sprintf(pipe_tmp, "pipe%d", Ide);//casting
    
    if (kill(pidDest, SIGUSR1)) {
      perror("Error mandando la señal");
    } else {
      printf("\nse le notifico al talker\n\n");
    }
    
    sprintf(res, "\"%s\" desde %d", envio, req.ID);
    
    int fd_tmp = open(pipe_tmp, O_WRONLY);
    
    if (write(fd_tmp, &res, sizeof(res)) == 0) {
      perror("Error en la escritura");
    }
    
    close(fd_tmp);
    
  } else {
    
    int pid_tmp = pidTalker(req.ID);

    if (kill(pid_tmp, SIGUSR1)) {
      perror("Error mandando la señal");
    } else {
      printf("\nse le notifico al talker\n\n");
    }
    
    sprintf(pipe_tmp, "pipe%d", req.ID);//casting
    int fd_tmp = open(pipe_tmp, O_WRONLY);
    
    char env[] = "El ID seleccionado no existe";
    if (write(fd_tmp, &env, sizeof(env)) == 0){
      perror("Error en la escritura");
      exit(1);
    }

    close(fd_tmp);
  }
}

void sent_group(Request req) {

  char pipe_tmp[7], res[max_res];

  char *envio = strtok(req.args, "\"");
  char *g = strtok(NULL," G");
  int dir = atoi(g);
  int pidtmp;
  
  if (dir >= c_groups) {
    char error[max_res];
    sprintf(error, "Error: El grupo 'G%d' no existe ", dir);
    
    int pid_tmp = pidTalker(req.ID);

    if (kill(pid_tmp, SIGUSR1)) {
      perror("Error mandando la señal");
    } else {
      printf("\nse le notifico al talker\n\n");
    }

    sprintf(pipe_tmp, "pipe%d", req.ID);
    int fd_tmp = open(pipe_tmp, O_WRONLY);
  
    if (write(fd_tmp, &error, sizeof(error)) == 0) {
      perror("Error en la escritura");
      exit(1);

    }
    close(fd_tmp);
    return;
    
  }
  strcat(envio, " desde G");

  sprintf(res, "\"%s%d\"", envio, dir);
  
  int check = 0;

  for (int i = 0; i < c_talkers; i++){
    if (groups[dir].ID_talkers[i] == req.ID){
      check = 1;
      break;
    }
  }
  
  if (check) {
    for (int i = 0; i < groups[dir].c_talkers; i++) {
      if (status_t(groups[dir].ID_talkers[i]) == 'c' && req.ID != groups[dir].ID_talkers[i]) {
  
        pidtmp = pidTalker(groups[dir].ID_talkers[i]);
  
        if (kill(pidtmp, SIGUSR1)) {
          perror("Error mandando la señal");
        } else {
          printf("\nse le notifico al talker\n\n");
        }
  
        sprintf(pipe_tmp, "pipe%d", groups[dir].ID_talkers[i]);
        
        int fd_tmp = open(pipe_tmp, O_WRONLY);
  
        if (write(fd_tmp, &res, sizeof(res)) == 0) {
          perror("Error en la escritura");
          exit(1);
        }
        
        close(fd_tmp);
      }
    }
  } else {
    char error[max_res];

    sprintf(error, "Error: No perteneces el grupo 'G%d'", dir);
    
    int pid_tmp = pidTalker(req.ID);

    if (kill(pid_tmp, SIGUSR1)) {
      perror("Error mandando la señal");
    } else {
      printf("\nse le notifico al talker\n\n");
    }

    sprintf(pipe_tmp, "pipe%d", req.ID);
    int fd_tmp = open(pipe_tmp, O_WRONLY);
  
    if (write(fd_tmp, &error, sizeof(error)) == 0) {
      perror("Error en la escritura");
      exit(1);
    }
    close(fd_tmp);
  }
}

int pidTalker(int ID) {
  int pidDest = -1;
  
  for (int i = 0; i < c_talkers; i++) {
    if (ID == talkers[i].ID && talkers[i].conectado) {
      pidDest = talkers[i].pid;
      break;
    }
  }
  
  return pidDest;
}

// n: si no existe, e: si existe y no esta conectado, y c: si existe y esta conectado
char status_t(int ID) {

  int valor = 0, lugar;
  for (int i = 0; i < c_talkers; i++) {
    if (talkers[i].ID == ID) {
      valor = 1;
      lugar = i;
      break;
    }
  }

  if (valor == 1) {
    if (talkers[lugar].conectado == 1)
      return 'c';
    else
      return 'e';
  } else {
    return 'n';
  }
}

int match(char *regex, char *str) {
  regex_t r;
  if (regcomp(&r, regex, REG_EXTENDED) != 0) {
    perror("Error al compilar el regex");
    exit(EXIT_FAILURE);
  }
  if (regexec(&r, str, 0, NULL, 0) == 0) {
    regfree(&r);
    return 1;
  } else {
    return 0;
  }
}
typedef struct Request {
  int ID;
  char type[10];
  char args[200];
  int c_args;
} Request;

typedef struct Group {
  int ID;
  int *ID_talkers;
  int c_talkers;
} Group;

typedef struct Talker {
  int ID;
  int pid;
  int conectado;
} Talker;
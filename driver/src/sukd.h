#define RPI_REV 1;

//if (RPI_REV == 1) define("PIN_TAMP2", 21 );
//else define("PIN_TAMP2", 27 );

int main(int argc, char *argv[]);
int parse_command(char *buf, char *cmd, char *param, int len);

void catch_terminate(int sig);

void *tstatus(void *arg);
void *tcommands(void *arg);
void *tsukeklis(void *arg);

#include "displayer.h"

static struct proc_register proc;

void proc_reg(int proc_id){
    proc.proc[proc.count++] = proc_id;
}

void handle_segv(int num){
    for (int i = 0; i < proc.count; i++){
        kill(proc.proc[i], SIGTERM);
    }

    _exit(0);
}

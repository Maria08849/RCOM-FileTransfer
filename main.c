#include "application_layer.h"

int main(int argc, char* argv[]){

    if (argc < 6) {
        printf("Insufficient arguments provided.\n");
        return 1;
    }

    char* port = argv[1];
    char* filename;
    int mode;
    if (strcmp(argv[2], "TRANSMITER") == 0)
        mode = TRANSMITER;
    else if (strcmp(argv[2], "RECEIVER") == 0)
        mode = RECEIVER;
    else {
        printf("Invalid mode provided.\n");
        return 1;
    }
    
    int baudrate = atoi(argv[3]);
    int n_tries = atoi(argv[4]);
    int timeout = atoi(argv[5]);

    if (mode == TRANSMITER)
        filename = argv[6];
    else
        filename = "";

    applicationLayer(port, mode, baudrate, n_tries, timeout, filename);

    return 0;
}

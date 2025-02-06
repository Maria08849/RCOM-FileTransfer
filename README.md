# Image Transfer Program

## Overview

This repository hosts a C program designed for file transfer between two computers. The development of this program is part of the curriculum for the [Computer Networks](https://sigarra.up.pt/feup/en/UCURR_GERAL.FICHA_UC_VIEW?pv_ocorrencia_id=520330) course.

## Usage

To compile and execute the code along with all its tests, use the following commands:

```sh
make main
./main <port> <filename> <mode> <baudrate> <n_tries> <timeout>
```

Mode:

* **TRANSMITER:** The computer that will transmit the file
* **RECEIVER:** THE computer that receives the file

This will build and run the program.
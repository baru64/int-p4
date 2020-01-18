#ifndef EXPORTER_H
#define EXPORTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "util.h"
#include "parser.h"

// TODO implement hash map

void* report_exporter(void* args);

#endif

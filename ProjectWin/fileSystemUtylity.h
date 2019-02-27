#pragma once

//
// Created by alfylinux on 06/07/18.
//

#ifndef CLIENT_FILESYSTEMUTYLITY_H
#define CLIENT_FILESYSTEMUTYLITY_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <dirent.h>
#include <errno.h>
#include <time.h>
//#include <semaphore.h>

#include <Windows.h>

#include "globalSet.h"
#include "tableFile.h"
#include "mexData.h"



/** PROTOTIPI   **/

///CLIENT
// Inizializzazione client
int StartClientStorage(char *storage_name);

#endif //CLIENT_FILESYSTEMUTYLITY_H

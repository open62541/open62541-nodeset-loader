/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend/backend.h"
#include "nodesetLoader.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("specify nodesetfile as argument. E.g. parserDemo text.xml\n");
        return -1;
    }
    FileHandler handler;
    handler.file = argv[1];
    handler.callback = addNode;
    handler.addNamespace = addNamespace;
    handler.userContext = NULL;
    loadFile(&handler);
}

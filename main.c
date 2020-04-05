/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include "backend/backend.h"
#include <nodesetLoader/nodesetLoader.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("specify nodesetfile as argument. E.g. parserDemo text.xml\n");
        return 1;
    }
    FileContext handler;
    handler.callback = addNode;
    handler.addNamespace = addNamespace;
    handler.userContext = NULL;
    ValueInterface valIf;
    valIf.userData = NULL;
    valIf.newValue = Value_new;
    valIf.start = Value_start;
    valIf.end = Value_end;
    valIf.finish = Value_finish;
    valIf.deleteValue = Value_delete;
    handler.valueHandling = &valIf;

    for(int cnt = 1; cnt < argc; cnt++) {
        handler.file = argv[cnt];
        if(!loadFile(&handler)) {
            printf("nodeset could not be loaded, exit\n");
            return 1;
        }
    }
    return 0;
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <NodesetLoader/NodesetLoader.h>

static void
logger(void *context, enum NodesetLoader_LogLevel level,
       const char *message, ...) {
    (void)context;
    (void)level;
    (void)message;
}

static void
addNamespace(void *context, size_t urisSize, UA_String *uris,
             UA_NamespaceMapping *mapping) {
    (void)context;
    (void)urisSize;
    (void)uris;
    (void)mapping;
}

static bool
stringEqual(const UA_String *value, const char *expected) {
    size_t length = strlen(expected);
    return value->length == length && !memcmp(value->data, expected, length);
}

typedef struct {
    bool objectSeen;
    bool variableSeen;
    bool failed;
} TestContext;

static bool
checkNode(void *context, NL_Node *node) {
    TestContext *test = (TestContext*)context;
    if(node->id.identifierType != UA_NODEIDTYPE_NUMERIC)
        return true;

    if(node->id.identifier.numeric == 50000) {
        test->objectSeen = true;
        if(!stringEqual(&node->browseName.name, "A&B"))
            fprintf(stderr, "BrowseName was not entity-decoded\n");
        if(!stringEqual(&node->displayName.locale, "en&US"))
            fprintf(stderr, "Locale was not entity-decoded\n");
        if(!stringEqual(&node->displayName.text, "A < B & C"))
            fprintf(stderr, "DisplayName was not entity-decoded\n");
        test->failed |= !stringEqual(&node->browseName.name, "A&B") ||
            !stringEqual(&node->displayName.locale, "en&US") ||
            !stringEqual(&node->displayName.text, "A < B & C");
    } else if(node->id.identifier.numeric == 50001) {
        test->variableSeen = true;
        const NL_VariableNode *variable = (const NL_VariableNode*)node;
        if(!stringEqual(
               &variable->value,
               "<x:Value><uax:String>hello &amp; goodbye</uax:String></x:Value>")) {
            fprintf(stderr, "Raw Value was not preserved: %.*s\n",
                    (int)variable->value.length, variable->value.data);
            test->failed = true;
        }
        if(!stringEqual(&node->description.text, "After Value")) {
            fprintf(stderr, "Element after Value was not parsed\n");
            test->failed = true;
        }
    } else if(node->id.identifier.numeric == 59999) {
        fprintf(stderr, "Node below an unknown element was imported\n");
        test->failed = true;
    }
    return !test->failed;
}

static bool
parseValid(const char *directory) {
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/parser.xml", directory);
    if(written < 0 || (size_t)written >= sizeof(path))
        return false;

    NodesetLoader_Logger log = {NULL, logger};
    NodesetLoader *loader = NodesetLoader_new(&log);
    if(!loader)
        return false;

    NL_FileContext file;
    memset(&file, 0, sizeof(file));
    UA_NamespaceMapping mapping;
    memset(&mapping, 0, sizeof(mapping));
    file.file = path;
    file.addNamespace = addNamespace;
    file.nsMapping = &mapping;
    bool success = NodesetLoader_importFile(loader, &file);

    TestContext context;
    memset(&context, 0, sizeof(context));
    if(success)
        success = NodesetLoader_sort(loader);
    if(success)
        success = NodesetLoader_forEachNode(loader, &context, checkNode);
    success = success && context.objectSeen && context.variableSeen &&
        !context.failed;
    NodesetLoader_delete(loader);
    UA_NamespaceMapping_clear(&mapping);
    return success;
}

static bool
rejectMalformed(const char *directory) {
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/parser_malformed.xml",
                           directory);
    if(written < 0 || (size_t)written >= sizeof(path))
        return false;

    NodesetLoader_Logger log = {NULL, logger};
    NodesetLoader *loader = NodesetLoader_new(&log);
    if(!loader)
        return false;

    NL_FileContext file;
    memset(&file, 0, sizeof(file));
    UA_NamespaceMapping mapping;
    memset(&mapping, 0, sizeof(mapping));
    file.file = path;
    file.addNamespace = addNamespace;
    file.nsMapping = &mapping;
    bool rejected = !NodesetLoader_importFile(loader, &file);
    NodesetLoader_delete(loader);
    UA_NamespaceMapping_clear(&mapping);
    return rejected;
}

int
main(int argc, char **argv) {
    if(argc != 2)
        return EXIT_FAILURE;
    if(!parseValid(argv[1]))
        return 2;
    if(!rejectMalformed(argv[1]))
        return 3;
    return EXIT_SUCCESS;
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "NodesetLoader.h"

#include <open62541/server_config_default.h>

#include <stdio.h>
#include <string.h>

typedef struct
{
    UA_Server *server;
    UA_NodeSetLoaderContext context;
    UA_NodeSetLoader *loader;
} ParserFixture;

static void ParserFixture_clear(ParserFixture *fixture)
{
    UA_NodeSetLoader_delete(fixture->loader);
    UA_NodeSetLoaderContext_clear(&fixture->context);
    UA_Server_delete(fixture->server);
}

static bool ParserFixture_init(ParserFixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->server = UA_Server_new();
    if (!fixture->server)
        return false;
    UA_ServerConfig *config = UA_Server_getConfig(fixture->server);
    if (UA_ServerConfig_setDefault(config) != UA_STATUSCODE_GOOD)
    {
        ParserFixture_clear(fixture);
        return false;
    }
    if (UA_NodeSetLoaderContext_init(&fixture->context, fixture->server,
                                     config->logging) != UA_STATUSCODE_GOOD)
    {
        ParserFixture_clear(fixture);
        return false;
    }
    fixture->loader = UA_NodeSetLoader_new(&fixture->context);
    if (fixture->loader)
        return true;
    ParserFixture_clear(fixture);
    return false;
}

static UA_StatusCode importPath(UA_NodeSetLoader *loader, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return UA_STATUSCODE_BADNOTFOUND;
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString xml = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_ByteString_allocBuffer(&xml, (size_t)length);
    if (status == UA_STATUSCODE_GOOD &&
        fread(xml.data, 1, xml.length, file) != xml.length)
        status = UA_STATUSCODE_BADINTERNALERROR;
    fclose(file);

    if (status == UA_STATUSCODE_GOOD)
        status = UA_NodeSetLoader_import(loader, &xml);
    UA_ByteString_clear(&xml);
    return status;
}

static bool stringEqual(const UA_String *value, const char *expected)
{
    size_t length = strlen(expected);
    return value->length == length && !memcmp(value->data, expected, length);
}

typedef struct
{
    bool objectSeen;
    bool variableSeen;
    bool memberTypeSeen;
    bool dependentTypeSeen;
    bool failed;
} TestContext;

static bool checkNode(void *context, NL_Node *node)
{
    TestContext *test = (TestContext *)context;
    if (node->id.identifierType != UA_NODEIDTYPE_NUMERIC)
        return true;

    if (node->id.identifier.numeric == 50002)
    {
        test->memberTypeSeen = true;
    }
    else if (node->id.identifier.numeric == 50003)
    {
        test->dependentTypeSeen = true;
        if (!test->memberTypeSeen)
        {
            fprintf(stderr,
                    "Dependent datatype was sorted before its member\n");
            test->failed = true;
        }
    }
    else if (node->id.identifier.numeric == 50000)
    {
        test->objectSeen = true;
        if (!stringEqual(&node->browseName.name, "A&B"))
            fprintf(stderr, "BrowseName was not entity-decoded\n");
        if (!stringEqual(&node->displayName.locale, "en&US"))
            fprintf(stderr, "Locale was not entity-decoded\n");
        if (!stringEqual(&node->displayName.text, "A < B & C"))
            fprintf(stderr, "DisplayName was not entity-decoded\n");
        test->failed |= !stringEqual(&node->browseName.name, "A&B") ||
                        !stringEqual(&node->displayName.locale, "en&US") ||
                        !stringEqual(&node->displayName.text, "A < B & C");
    }
    else if (node->id.identifier.numeric == 50001)
    {
        test->variableSeen = true;
        const NL_VariableNode *variable = (const NL_VariableNode *)node;
        if (!stringEqual(&variable->value, "<x:Value><uax:String>hello &amp; "
                                           "goodbye</uax:String></x:Value>"))
        {
            fprintf(stderr, "Raw Value was not preserved: %.*s\n",
                    (int)variable->value.length, variable->value.data);
            test->failed = true;
        }
        if (!stringEqual(&node->description.text, "After Value"))
        {
            fprintf(stderr, "Element after Value was not parsed\n");
            test->failed = true;
        }
    }
    else if (node->id.identifier.numeric == 59999)
    {
        fprintf(stderr, "Node below an unknown element was imported\n");
        test->failed = true;
    }
    return !test->failed;
}

static bool parseValid(const char *directory)
{
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/parser.xml", directory);
    if (written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if (!ParserFixture_init(&fixture))
        return false;

    bool success = importPath(fixture.loader, path) == UA_STATUSCODE_GOOD;

    TestContext context;
    memset(&context, 0, sizeof(context));
    if (success)
        success = UA_NodeSetLoader_sort(fixture.loader);
    if (success)
        success = UA_NodeSetLoader_forEach(fixture.loader, &context, checkNode);
    success = success && context.objectSeen && context.variableSeen &&
              context.memberTypeSeen && context.dependentTypeSeen &&
              !context.failed;
    ParserFixture_clear(&fixture);
    return success;
}

static bool rejectMalformed(const char *directory)
{
    char path[1024];
    int written =
        snprintf(path, sizeof(path), "%s/parser_malformed.xml", directory);
    if (written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if (!ParserFixture_init(&fixture))
        return false;

    bool rejected = importPath(fixture.loader, path) != UA_STATUSCODE_GOOD;
    ParserFixture_clear(&fixture);
    return rejected;
}

static bool rejectInvalidNodes(const char *directory)
{
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/invalidNodeDefinitions.xml",
                           directory);
    if (written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if (!ParserFixture_init(&fixture))
        return false;

    bool rejected = importPath(fixture.loader, path) != UA_STATUSCODE_GOOD;
    ParserFixture_clear(&fixture);
    return rejected;
}

static bool rejectUnsortable(const char *directory)
{
    char path[1024];
    int written = snprintf(path, sizeof(path), "%s/unsortable.xml", directory);
    if (written < 0 || (size_t)written >= sizeof(path))
        return false;

    ParserFixture fixture;
    if (!ParserFixture_init(&fixture))
        return false;

    bool success = importPath(fixture.loader, path) == UA_STATUSCODE_GOOD;
    bool rejected = success && !UA_NodeSetLoader_sort(fixture.loader);
    ParserFixture_clear(&fixture);
    return rejected;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return EXIT_FAILURE;
    if (!parseValid(argv[1]))
        return 2;
    if (!rejectMalformed(argv[1]))
        return 3;
    if (!rejectInvalidNodes(argv[1]))
        return 4;
    if (!rejectUnsortable(argv[1]))
        return 5;
    return EXIT_SUCCESS;
}

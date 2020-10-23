static void cleanupCustomTypes(const UA_DataTypeArray *types)
{
    while (types)
    {
        const UA_DataTypeArray *next = types->next;
        if (types->types)
        {
            for (const UA_DataType *type = types->types;
                 type != types->types + types->typesSize; type++)
            {
                free((void*)(uintptr_t)type->typeName);
                UA_UInt32 mSize =
                    type->membersSize;
                if(type->typeKind == UA_DATATYPEKIND_UNION)
                {
                    mSize--;
                }
                for (UA_DataTypeMember *m = type->members;
                                           m !=
                                           type->members + mSize;
                                           m++)
                {
                    free((void *)m->memberName);
                    m->memberName = NULL;
                }
                free(type->members);
            }
        }
        free((void *)(uintptr_t)types->types);
        free((void *)types);
        types = next;
    }
}

UA_NodeId getTypeId(UA_UInt16 typeIndex, UA_Boolean isNamespaceZero, const UA_DataType* customTypes)
{
    const UA_DataType* types[2] = {&UA_TYPES[0], customTypes};
    return types[!isNamespaceZero][typeIndex].typeId;
}

static void memberTypeMatching(const UA_DataTypeMember* m1, const UA_DataTypeMember* m2, const UA_DataType* generatedTypes, const UA_DataType* customTypes)
{
    ck_assert(m1->isArray == m2->isArray);
    ck_assert(m1->namespaceZero == m2->namespaceZero);
    ck_assert(m1->padding == m2->padding);
    ck_assert(m1->isOptional == m2->isOptional);
    UA_NodeId m1Id = getTypeId(m1->memberTypeIndex, m1->namespaceZero, generatedTypes);
    UA_NodeId m2Id = getTypeId(m2->memberTypeIndex, m2->namespaceZero, customTypes);
    ck_assert(UA_NodeId_equal(&m1Id, &m2Id));
}

void typesAreMatching(const UA_DataType *t1, const UA_DataType *t2, const UA_DataType* generatedTypes, const UA_DataType* customTypes)
{
    ck_assert(UA_NodeId_equal(&t1->binaryEncodingId, &t2->binaryEncodingId));
    ck_assert(t1->membersSize == t2->membersSize);
    ck_assert(t1->memSize == t2->memSize);
    ck_assert(t1->overlayable == t2->overlayable);
    ck_assert(t1->pointerFree == t2->pointerFree);
    ck_assert(t1->typeKind == t2->typeKind);
    ck_assert(!strcmp(t1->typeName, t2->typeName));
    size_t cnt =0;
    UA_UInt32 mSize = t1->membersSize;
    if (t1->typeKind == UA_DATATYPEKIND_UNION)
    {
        mSize--;
    }
    for(const UA_DataTypeMember* m = t1->members; m!=t1->members+mSize; m++)
    {
        memberTypeMatching(m, &t2->members[cnt], generatedTypes, customTypes);
        cnt++;
    }
}

UA_NodeClass getNodeClass(UA_Server* server, const UA_NodeId id)
{
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode status =
        UA_Server_readNodeClass(server, id, &nodeClass);
    ck_assert(UA_STATUSCODE_GOOD == status);
    return nodeClass;
}

UA_Boolean hasReference(UA_Server* server, const UA_NodeId src, const UA_NodeId target, const UA_NodeId refType, UA_BrowseDirection dir)
{
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.referenceTypeId = refType;
    bd.browseDirection = dir;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.nodeId = src;
    UA_BrowseResult br = UA_Server_browse(server, 10, &bd);
    ck_assert(br.statusCode == UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize == 1);
    UA_Boolean targetMatches = UA_NodeId_equal(&target, &br.references[0].nodeId.nodeId);
    UA_BrowseResult_clear(&br);
    return targetMatches;
}


UA_NodeId getTypeDefinitionId(UA_Server *s, const UA_NodeId targetId)
{
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = UA_FALSE;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    bd.nodeId = targetId;
    bd.resultMask = UA_BROWSERESULTMASK_TYPEDEFINITION;
    UA_BrowseResult br = UA_Server_browse(s, 10, &bd);
    if (br.statusCode != UA_STATUSCODE_GOOD || br.referencesSize != 1)
    {
        return UA_NODEID_NULL;
    }
    UA_NodeId id = br.references->nodeId.nodeId;
    UA_BrowseResult_clear(&br);
    return id;
}
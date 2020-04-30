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
                // free(type->typeName);
                for (UA_DataTypeMember *m = type->members;
                     m != type->members + type->membersSize; m++)
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

static void memberTypeMatching(const UA_DataTypeMember* m1, const UA_DataTypeMember* m2)
{
    ck_assert(m1->isArray == m2->isArray);
    ck_assert(m1->namespaceZero == m2->namespaceZero);
    ck_assert(m1->padding == m2->padding);
    //todo: membertype should be checked
}

void typesAreMatching(const UA_DataType *t1, const UA_DataType *t2)
{
    ck_assert(t1->binaryEncodingId == t2->binaryEncodingId);
    ck_assert(t1->membersSize == t2->membersSize);
    ck_assert(t1->memSize == t2->memSize);
    ck_assert(t1->overlayable == t2->overlayable);
    ck_assert(t1->pointerFree == t2->pointerFree);
    ck_assert(t1->typeKind == t2->typeKind);
    size_t cnt =0;
    for(const UA_DataTypeMember* m = t1->members; m!=t1->members+t1->membersSize; m++)
    {
        memberTypeMatching(m, &t2->members[cnt]);
        cnt++;
    }
}
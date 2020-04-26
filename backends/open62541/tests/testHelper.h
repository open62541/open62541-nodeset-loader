static void cleanupCustomTypes(UA_DataTypeArray *types)
{
    while (types)
    {
        UA_DataTypeArray *next = types->next;
        if (types->types)
        {
            for (UA_DataType *type = types->types;
                 type != types->types + types->typesSize; type++)
            {
                //free(type->typeName);
                for (UA_DataTypeMember *m = type->members;
                     m != type->members + type->membersSize; m++)
                {
                    free(m->memberName);
                }
            }
            free(types->types->members);
        }
        free((void *)(uintptr_t)types->types);
        free(types);
        types = next;
    }
}
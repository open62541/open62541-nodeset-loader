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
                //free(type->typeName);
                for (UA_DataTypeMember *m = type->members;
                     m != type->members + type->membersSize; m++)
                {
                    free((void*)m->memberName);
                }
                free(type->members);
            }
        }
        free((void *)(uintptr_t)types->types);
        free((void*)types);
        types = next;
    }
}
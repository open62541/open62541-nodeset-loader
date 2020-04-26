#ifndef TNODEID_H
#define TNODEID_H
typedef struct
{
    int nsIdx;
    char *id;
} TNodeId;
int TNodeId_cmp(const TNodeId *id1, const TNodeId *id2);
#endif
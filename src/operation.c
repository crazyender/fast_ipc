#include <fipc.h>
#include <internal.h>

extern fipc_op pipe_op;
extern fipc_op eventfd_op;
extern fipc_op spin_op;

static fipc_op *all_ops[] = {NULL, &pipe_op, &eventfd_op, &spin_op};

fipc_op *get_op(fipc_type type) { return all_ops[type]; }
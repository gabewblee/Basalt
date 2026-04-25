#pragma once

/* System config */
#define PG_DEF_SZ                        4096
#define DELETED_DEF_VAL                  0xDEADBEEF

/* Memtable config */
#define MEMTABLE_DEF_SZ                  1000

/* Buffer Pool config */
#define BUFFER_POOL_DEF_SZ               256
#define BUFFER_POOL_CAPACITY             10
#define BUFFER_POOL_LIMIT                1000

/* B+ Tree config */
#define BTREE_NODE_DEF_SZ                255

/* Filter config */
#define FILTER_TARGET_FPR                0.01

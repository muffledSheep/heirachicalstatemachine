#pragma once

#define DIE(...) ({fprintf(stderr, __VA_ARGS__); exit(1);})

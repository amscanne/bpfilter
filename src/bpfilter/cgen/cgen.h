/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Meta Platforms, Inc. and affiliates.
 */

#pragma once

#include <stdint.h>

#include "core/counter.h"
#include "core/dump.h"
#include "core/front.h"

struct bf_chain;
struct bf_marsh;
struct bf_program;

#define _cleanup_bf_cgen_ __attribute__((cleanup(bf_cgen_free)))

/**
 * Convenience macro to initialize a list of @ref bf_cgen .
 *
 * @return An initialized @ref bf_list that can contain @ref bf_cgen objects.
 */
#define bf_cgen_list()                                                         \
    ((bf_list) {.ops = {.free = (bf_list_ops_free)bf_cgen_free,                \
                        .marsh = (bf_list_ops_marsh)bf_cgen_marsh}})

/**
 * @struct bf_cgen
 *
 * A codegen is a BPF bytecode generation context used to create a BPF program
 * for a given set of rules, sets, and policy (a chain).
 */
struct bf_cgen
{
    /// Front used to define the chain.
    enum bf_front front;

    /// Chain containing the rules, sets, and policy.
    struct bf_chain *chain;

    /// Program generated by the codegen.
    struct bf_program *program;
};

/**
 * Allocate and initialise a new codegen.
 *
 * @param cgen Codegen to allocate and initialise. Can't be NULL.
 * @param front Front used to define the chain.
 * @param chain Chain containing the codegen's rules, sets, and policy. On
 *        success, the new codegen will take ownership of the chain, and
 *        @c *chain will be NULL. Can't be NULL, and @c *chain must point to
 *        a valid @ref bf_chain .
 * @return 0 on success, or negative errno value on failure.
 */
int bf_cgen_new(struct bf_cgen **cgen, enum bf_front front,
                struct bf_chain **chain);

/**
 * Allocate a new codegen and intialize it from serialized data.
 *
 * @param cgen Codegen to allocate and initialize. On success, @p *cgen will
 *        point to the new codegen object. On failure, @p *cgen is unchanged.
 *        Can't be NULL.
 * @param marsh Serialized data to use to initialize the codegen.
 * @return 0 on success, or negative errno value on error.
 */
int bf_cgen_new_from_marsh(struct bf_cgen **cgen, const struct bf_marsh *marsh);

/**
 * Free a codegen.
 *
 * If one or more programs are loaded, they won't be unloaded. Use @ref
 * bf_cgen_unload first to ensure programs are unloaded. This behaviour
 * is expected so @ref bf_cgen can be freed without unloading the BPF
 * program, during a daemon restart for example.
 *
 * @param cgen Codegen to free. Can't be NULL.
 */
void bf_cgen_free(struct bf_cgen **cgen);

/**
 * Serialize a @ref bf_cgen object.
 *
 * @param cgen Codegen object to serialize. Can't be NULL.
 * @param marsh Marsh object to allocate. On success, @p *marsh points to the
 *              serialized codegen object. On failure this parameter is
 *              unchanged. Can't be NULL.
 * @return 0 on success, or a negative errno value on failure.
 */
int bf_cgen_marsh(const struct bf_cgen *cgen, struct bf_marsh **marsh);

/**
 * Update the BPF programs for a codegen.
 *
 * @param cgen Codegen to update. Can't be NULL.
 * @param chain Chain containing the new rules, sets, and policy. On success,
 *        @c *chain will be replaced by the previous (old) chain: the codegen
 *        will take ownership of the new chain and the caller will be
 *        responsible for freeing the old one. Can't be NULL, @c *chain must
 *        point to a valid @ref bf_chain .
 * @return 0 on success, or negative errno value on failure.
 */
int bf_cgen_update(struct bf_cgen *cgen, struct bf_chain **chain);

/**
 * Create a @ref bf_program for each interface, generate the program, load it,
 * and attach it to the kernel.
 *
 * Simplify @ref bf_program management by providing a single call to add the
 * programs to the systems, starting from a new @ref bf_cgen.
 *
 * @param cgen Codegen to generate the programs for, and load to the system.
 * @return 0 on success, or negative errno value on failure.
 */
int bf_cgen_up(struct bf_cgen *cgen);

/**
 * Unload a codegen's BPF programs.
 *
 * @param cgen Codegen containing the BPF program to unload. Can't be NULL.
 * @return 0 on success, negative error code on failure.
 */
int bf_cgen_unload(struct bf_cgen *cgen);

void bf_cgen_dump(const struct bf_cgen *cgen, prefix_t *prefix);

/**
 * @enum bf_counter_type
 *
 * Special counter types for @ref bf_cgen_get_counter .
 */
enum bf_counter_type
{
    BF_COUNTER_POLICY = -1,
    BF_COUNTER_ERRORS = -2,
};

/**
 * Get packets and bytes counter at a specific index.
 *
 * Counters are referenced by their index in the counters map or the enum
 * values defined by @ref bf_counter_type .
 *
 * The counter from all the program generated from @p cgen are summarised
 * together.
 *
 * @param cgen Codegen to get the counter for. Can't be NULL.
 * @param counter_idx Index of the counter to get. If @p counter_idx doesn't
 *        correspond to a valid index, -E2BIG is returned.
 * @param counter Counter structure to fill with the counter values. Can't be
 *        NULL.
 * @return 0 on success, or a negative errno value on failure.
 */
int bf_cgen_get_counter(const struct bf_cgen *cgen,
                        enum bf_counter_type counter_idx,
                        struct bf_counter *counter);

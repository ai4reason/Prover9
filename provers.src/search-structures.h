// File: search-structures.h

// ***************************************************************************
// Log of changes made by BV (newest first):
//
//    BV(2017-apr-03): New parm, hint_derivations
//
//    BV(2017-jan-22): New parms and exit code
//
//       max_nohints
//       degrade_limit
//  
//       exit code MAX_NOHINTS_EXIT
//
//    BV(2016-nov-08): New flag, print_processed_hints.
//
//    BV(2016-nov-02): New input formulas list "unused".
//
//    BV(2016-aug-25): New flags
//
//       print_derivations
//       derivations_only
//
//    BV(2015-jan-31): New flag, print_matched_hints.
// ***************************************************************************

/*  Copyright (C) 2006, 2007 William McCune

    This file is part of the LADR Deduction Library.

    The LADR Deduction Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License,
    version 2.

    The LADR Deduction Library is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the LADR Deduction Library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TP_SEARCH_STRUCTURES_H
#define TP_SEARCH_STRUCTURES_H

#include "../ladr/ladr.h"

// Attributes

typedef struct prover_attributes * Prover_attributes;

struct prover_attributes {
  // These are attribute IDs
  int label,
    bsub_hint_wt,
    answer,
    properties,
    action,
    action2;
};

// Options

typedef struct prover_options * Prover_options;

struct prover_options {
  // These are integer IDs assigned by init_flag(), init_parm, init_stringparm.

  // Flags (Boolean options)
  int 
    binary_resolution,       // inference rules
    neg_binary_resolution,
    hyper_resolution,
    pos_hyper_resolution,
    neg_hyper_resolution,
    ur_resolution,
    pos_ur_resolution,
    neg_ur_resolution,
    paramodulation,

    ordered_res,        // restrictions on inference rules
    ordered_para,
    check_res_instances,
    check_para_instances,
    para_units_only,
    para_from_vars,
    para_into_vars,
    para_from_small,
    basic_paramodulation,
    initial_nuclei,

    process_initial_sos,     // processing generated clauses
    back_demod,
    lex_dep_demod,
    lex_dep_demod_sane,
    safe_unit_conflict,
    reuse_denials,
    back_subsume,
    unit_deletion,
    factor,
    cac_redundancy,
    degrade_hints,
    limit_hint_matchers,
    back_demod_hints,
    collect_hint_labels,
    dont_flip_input,
    eval_rewrite,

    echo_input,              // output
    bell,
    quiet,
    print_initial_clauses,
    print_given,
    print_gen,
    print_kept,
    print_labeled,
    print_proofs,

    print_matched_hints,     // BV(2015-jan-31)
    print_derivations,       // BV(2016-aug-25)
    derivations_only,        // BV(2016-aug-25)
    print_processed_hints,   // BV(2016-nov-08)

    default_output,
    print_clause_properties,

    expand_relational_defs,
    predicate_elim,
    inverse_order,
    sort_initial_sos,
    restrict_denials,

    input_sos_first,        // selecting given clause
    breadth_first,
    lightest_first,
    default_parts,
    random_given,
    breadth_first_hints,

    automatic,             // auto is a reserved symbol in C!
    auto_setup,
    auto_limits,
    auto_denials,
    auto_inference,
    auto_process,
    auto2,                 // enhanced auto mode
    raw,
    production,

    lex_order_vars,       // others
    ignore_option_dependencies;
    
  // Parms (Integer options)

  int
    max_given,             // search limits
    max_kept,
    max_proofs,
    max_megs,
    max_seconds,
    max_minutes,
    max_hours,
    max_days,

    new_constants,        // inference
    para_lit_limit,
    ur_nucleus_limit,

    fold_denial_max,

    pick_given_ratio,      // select given clause
    hints_part,
    age_part,
    weight_part,
    true_part,
    false_part,
    random_part,
    random_seed,
    eval_limit,
    eval_var_limit,

    max_weight,            // processing generated clauses
    max_depth,
    lex_dep_demod_lim,
    max_literals,
    max_vars,
    demod_step_limit,
    demod_increase_limit,
    backsub_check,

    max_nohints,            // BV(2017-jan-22)
    degrade_limit,          // BV(2017-jan-22)
    hint_derivations,       // BV(2017-apr-03)

    variable_weight,        // weighting parameters
    constant_weight,
    not_weight,
    or_weight,
    sk_constant_weight,
    prop_atom_weight,
    nest_penalty,
    depth_penalty,
    var_penalty,
    default_weight,
    complexity,

    sos_limit,             // control size of SOS
    sos_keep_factor,
    min_sos_limit,
    lrs_interval,
    lrs_ticks,

    report,
    report_stderr;

  // Stringparms (string options)

  int
    order,               // LPO, RPO, KBO
    eq_defs,             // fold, unfold, pass
    literal_selection,   // maximal, etc.
    stats,               // none, some, lots, all
    multiple_interps;    // false_in_all, false_in_some
};

// Clocks

struct prover_clocks {
  Clock pick_given,
    infer,
    preprocess,
    demod,
    redundancy,
    unit_del,
    conflict,
    weigh,
    hints,
    subsume,
    semantics,
    back_subsume,
    back_demod,
    back_unit_del,
    index,
    disable;
};

// Statistics

typedef struct prover_stats * Prover_stats;

struct prover_stats {
  unsigned  given,
    generated,
    kept,
    proofs,
    kept_by_rule,
    deleted_by_rule,
    subsumed,
    back_subsumed,
    sos_limit_deleted,
    sos_displaced,
    sos_removed,
    new_demodulators,
    new_lex_demods,
    back_demodulated,
    back_unit_deleted,
    demod_attempts,
    demod_rewrites,
    res_instance_prunes,
    para_instance_prunes,
    basic_para_prunes,
    nonunit_fsub,
    nonunit_bsub,
    usable_size,
    sos_size,
    demodulators_size,
    disabled_size,
    hints_size,
    denials_size,
    limbo_size,
    kbyte_usage,
    new_constants;
};

// Search input

typedef struct prover_input * Prover_input;

struct prover_input {
  // tformula lists

  // BV(2016-nov-02): add input formulas list "unused"
  Plist usable, sos, demods, goals, hints, unused;

  // term lists
  Plist actions, weights, kbo_weights, interps;
  Plist given_selection, keep_rules, delete_rules;
  // ordinary options
  Prover_options options;
  // extra options
  BOOL xproofs;  // tell search() to return xproofs as well as ordinary proofs
};

// Search results

typedef struct prover_results * Prover_results;

struct prover_results {
  Plist proofs;
  Plist xproofs;
  struct prover_stats stats;
  double user_seconds, system_seconds;
  int return_code;
};

/* Exit codes */

enum {
  MAX_PROOFS_EXIT   = 0,
  FATAL_EXIT        = 1,  /* don't change this one! */
  SOS_EMPTY_EXIT    = 2,
  MAX_MEGS_EXIT     = 3,
  MAX_SECONDS_EXIT  = 4,
  MAX_GIVEN_EXIT    = 5,
  MAX_KEPT_EXIT     = 6,
  ACTION_EXIT       = 7,

  // BV(2017-jan-19): exit code for max_nohints
  MAX_NOHINTS_EXIT  = 8,

  SIGINT_EXIT       = 101,
  SIGSEGV_EXIT      = 102
};

#endif  /* conditional compilation of whole file */

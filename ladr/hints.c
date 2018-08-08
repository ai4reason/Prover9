// File: hints.c

// ***************************************************************************
// Log of changes (newest first):
//
//    BV(2017-jan-19): prep for user settable max FPA depth
//
//    JJ(2016-nov-10): Match any constant (_AnyConst) in hints
//      
//       Changes in index_hint so that Rules 1 & 2 described in term.c
//       are reflected.
//       1) Disable _AnyConst* matching when indexing hints (hints redundancy).
//       2) Disable demodulation of hints with generic _AnyConst.
//
//    BV(2016-aug-18): Include ioutil.h for call to fwrite_clause.
//
//    BV(2016-aug-18): Add doprint argument to function index_hint.
//
//       This is to distinguish the prover9 and prochints cases of
//       calls from search.c.
//
//    BV(2016-jun-17): Use hint id assigned at input time.
//
//    BV(2014-sep-08): Print clauses added to the hints index structure.
//
//       This version is used for the utility prochints, which starts off
//       like an ordinary prover9 job but prints the hints list after
//       processing and terminates.  This is a way to produce a cleaned
//       up set of hints for a series of prover9 runs.
//
//       See also file provers.src/search.c.
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

#include "hints.h"

// BV(2016-aug-18): for fwrite_clause
#include "ioutil.h"

/* Private definitions and types */

static Lindex Hints_idx = NULL;       /* FPA index for hints */
static Clist Redundant_hints = NULL;  /* list of hints not indexed */
static Mindex Back_demod_idx;        /* to index hints for back demodulation */
static int Bsub_wt_attr;
static BOOL Back_demod_hints;
static BOOL Collect_labels;

/* pointer to procedure for demodulating hints (when back demod hints) */

static void (*Demod_proc) (Topform, int, int, BOOL, BOOL);

/* stats */

static int Hint_id_count = 0;
static int Active_hints_count = 0;
static int Redundant_hints_count = 0;

/*************
 *
 *   init_hints()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void init_hints(Uniftype utype,
		int bsub_wt_attr,
		BOOL collect_labels,
		BOOL back_demod_hints,
		void (*demod_proc) (Topform, int, int, BOOL, BOOL))
{
  // BV(2017-jan-19):  on way to making hardwired constant user settable
  // static int max_fpa_depth = 10;  *** original value ***
  static int max_fpa_depth = 15;

  Bsub_wt_attr = bsub_wt_attr;
  Collect_labels = collect_labels;
  Back_demod_hints = back_demod_hints;
  Demod_proc = demod_proc;
  Hints_idx = lindex_init(FPA, utype, max_fpa_depth, FPA, utype, max_fpa_depth);
  if (Back_demod_hints)
    Back_demod_idx = mindex_init(FPA, utype, max_fpa_depth);
  Redundant_hints = clist_init("redundant_hints");
}  /* init_hints */

/*************
 *
 *   done_with_hints()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void done_with_hints(void)
{
  if (!lindex_empty(Hints_idx) ||
      !clist_empty(Redundant_hints))
    printf("ERROR: Hints index not empty!\n");
  lindex_destroy(Hints_idx);
  if (Back_demod_hints)
    mindex_destroy(Back_demod_idx);
  Hints_idx = NULL;
  clist_free(Redundant_hints);
  Redundant_hints = NULL;
}  /* done_with_hints */

/*************
 *
 *   redundant_hints()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
int redundant_hints(void)
{
  return clist_length(Redundant_hints);
}  /* redundant_hints */

/*************
 *
 *   find_equivalent_hint()
 *
 *************/

static
Topform find_equivalent_hint(Topform c, Lindex idx)
{
  Topform equiv_hint = NULL;
  Plist subsumees = back_subsume(c, idx);
  Plist p;
  for (p = subsumees; p && equiv_hint == NULL; p = p->next) {
    if (subsumes(p->v, c))
      equiv_hint = p->v;
  }
  zap_plist(subsumees);

  return equiv_hint;
}  /* find_equivalent_hint */

/*************
 *
 *   find_matching_hint()
 *
 *   Return the first equivalent hint;  if none, return the last
 *   subsumed hint.
 *
 *   "First" and "last" refer to the order returned by the index,
 *   which is not necessarily the order in which the hints were
 *   inserted into the index.  In fact, it is likely that the
 *   clauses are returned in the reverse order.
 *
 *************/

static
Topform find_matching_hint(Topform c, Lindex idx)
{
  //printf("FIND HINT FOR: "); p_clause(c); 

  Topform hint = NULL;
  Plist subsumees = back_subsume(c, idx);
  Plist p;
  BOOL equivalent = FALSE;
  for (p = subsumees; p && !equivalent; p = p->next) {
    /* printf("subsumee: "); f_clause(p->v); */
    hint = p->v;
    if (subsumes(p->v, c))
      equivalent = TRUE;
  }
  
  //printf("   FOUND HINT: "); p_clause(hint); 
  zap_plist(subsumees);
  return hint;
}  /* find_matching_hint */

/*************
 *
 *   index_hint()
 *
 *************/

/* DOCUMENTATION
Index a clause C as a hint (make sure to call init_hints first).
If the clause is equivalent to a previously indexed hint H, any
labels on C are copied to H, and C is not indexed.
*/

/* PUBLIC */
void index_hint(Topform c, BOOL doprint)
{
  /* JJ(2016-nov-10): Match any constant (_AnyConst) in hints 
   *
   * Temporarily turn off AnyConst matching so that hints with
   * AnyConst's are not marked redundant.
   *
   * */
  AnyConstsEnabled = FALSE;

  Topform h = find_equivalent_hint(c, Hints_idx);
  
  /* JJ(2016-nov-10): Match any constant (_AnyConst) in hints */
  AnyConstsEnabled = TRUE;

  c->weight = 0;  /* this is used in hints degradation to count matches */
  if (h != NULL) {
    /* copy any bsub_hint_wt attrs from rundundant hint to the indexed hint */
    h->attributes = copy_int_attribute(c->attributes, h->attributes,
				       Bsub_wt_attr);
    if (Collect_labels) {
      /* copy any labels from rundundant hint to the indexed hint */
      h->attributes = copy_string_attribute(c->attributes, h->attributes,
					    label_att());
    }
    clist_append(c, Redundant_hints);
    Redundant_hints_count++;
    /*
    printf("redundant hint: "); f_clause(c);
    printf("      original: "); f_clause(h);
    */
  }
  else {
    Active_hints_count++;
    Hint_id_count++;

    // BV(2016-jun-17): Use id assigned at input time.  Note that a back
    // demodulated hint will now keep the id of the original, which may be
    // necessary for avl_deletion for the hint_age given selection rule
    // (so the ordering function finds the clause in the avl tree).
 
    // c->id = Hint_id_count;  /* need IDs so that back_subsume() will work */

    lindex_update(Hints_idx, c, INSERT);
    if (Back_demod_hints) {
      
      /* JJ(2016-nov-10): Match any constant (_AnyConst) in hints 
       *
       * If c contains generic _AnyConst then do not index it in
       * back demodulation index.  This disables backward demodulation
       * of c.
       *
       * */
      if (MATCH_HINTS_ANYCONST) {
        I2list rsyms = NULL;
        I2list fsyms = NULL;
        gather_symbols_in_topform(c, &rsyms, &fsyms);
        I2list anyconst = i2list_member(fsyms, any_const_sn(0));
        zap_i2list(rsyms);
        zap_i2list(fsyms);
        if (anyconst) {
          return; 
        }
      }

      index_clause_back_demod(c, Back_demod_idx, INSERT);
    }

    /* --------------------------------------------------------------------*/
    /* BV(2014-sep-08): print clause added to hint index                  */ 

    if (doprint)
       fwrite_clause(stdout, c, CL_FORM_BARE);
    /* --------------------------------------------------------------------*/
  }
}  /* index_hint */

/*************
 *
 *   unindex_hint()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void unindex_hint(Topform c)
{
  if (clist_member(c, Redundant_hints)) {
    clist_remove(c, Redundant_hints);
    Redundant_hints_count--;
  }
  else {
    lindex_update(Hints_idx, c, DELETE);
    if (Back_demod_hints)
      index_clause_back_demod(c, Back_demod_idx, DELETE);
    Active_hints_count--;
  }
}  /* unindex_hint */

/*************
 *
 *   adjust_weight_with_hints()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void adjust_weight_with_hints(Topform c,
			      BOOL degrade,
			      BOOL breadth_first_hints)
{
  Topform hint = find_matching_hint(c, Hints_idx);

  if (hint == NULL &&
      unit_clause(c->literals) &&
      eq_term(c->literals->atom) &&
      !oriented_eq(c->literals->atom)) {

    /* Try to find a hint that matches the flipped equality. */

    Term save_atom = c->literals->atom;
    c->literals->atom = top_flip(save_atom);
    hint = find_matching_hint(c, Hints_idx);
    zap_top_flip(c->literals->atom);
    c->literals->atom = save_atom;
    if (hint != NULL)
      c->attributes = set_string_attribute(c->attributes, label_att(),
					   "flip_matches_hint");
  }

  if (hint != NULL) {

    int bsub_wt = get_int_attribute(hint->attributes, Bsub_wt_attr, 1);

    if (bsub_wt != INT_MAX)
      c->weight = bsub_wt;
    else if (breadth_first_hints)
      c->weight = 0;

    /* If the hint has label attributes, copy them to the clause. */
    
    {
      int i = 0;
      char *s = get_string_attribute(hint->attributes, label_att(), ++i);
      while (s) {
	if (!string_attribute_member(c->attributes, label_att(), s))
	  c->attributes = set_string_attribute(c->attributes, label_att(), s);
	s = get_string_attribute(hint->attributes, label_att(), ++i);
      }
    }

    /* Veroff's hint degradation strategy. */

    if (degrade) {
      /* for now, add 1000 for each previous match */
      int i;
      for (i = 0; i < hint->weight; i++) 
	c->weight = c->weight + 1000;
    }
    c->matching_hint = hint;
    /* If/when c is eventually kept, the hint will have its weight
       field incremented in case hint degradation is being used. */
  }
}  /* adjust_weight_with_hints */

/*************
 *
 *   keep_hint_matcher()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void keep_hint_matcher(Topform c)
{
  Topform hint = c->matching_hint;
  hint->weight++;
}  /* keep_hint_matcher */

/*************
 *
 *   back_demod_hints()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void back_demod_hints(Topform demod, int type, BOOL lex_order_vars)
{
  if (Back_demod_hints) {
    Plist rewritables = back_demod_indexed(demod, type, Back_demod_idx,
					   lex_order_vars);
    Plist p;
    for (p = rewritables; p; p = p->next) {
      Topform hint = p->v;
      /* printf("\nBEFORE: "); f_clause(hint); */
      unindex_hint(hint);
      (*Demod_proc)(hint, 1000, 1000, FALSE, lex_order_vars);

      orient_equalities(hint, TRUE);
      simplify_literals2(hint);
      merge_literals(hint);
      renumber_variables(hint, MAX_VARS);

      /* printf("AFTER : "); f_clause(hint); */
      index_hint(hint, FALSE);
      hint->weight = 0;  /* reset count of number of matches */
    }
  }
}  /* back_demod_hints */


// BV(2017-nov-12):  print indexed hints
void flag_indexed_hints()
{
   flag_fpa_clauses(stdout, Hints_idx -> pos -> fpa);
}

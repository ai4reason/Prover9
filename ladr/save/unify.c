#include "unify.h"

/* Private definitions and types */

/* A Trail records substitutions so that they can be easily undone.
 * Whenever a variable is instantiated (by making in entry in a
 * Context), one of these nodes is prepended to the current trail.
 */

struct trail {
  int     varnum;   /* index of instantiated variable */
  Context context;  /* context of instanteated variable */
  Trail   next;     /* next (earlier) member of trail */
};

/* bind a variable, record binding in a trail */

#define BIND_TR(i, c1, t2, c2, trp) { struct trail *tr; \
    c1->terms[i] = t2; c1->contexts[i] = c2; \
    tr = get_trail(); tr->varnum = i; tr->context = c1; \
    tr->next = *trp; *trp = tr; }

#define MAX_MULTIPLIERS 500

/* Private variables */

static BOOL Multipliers[MAX_MULTIPLIERS];  /* (m[i]==FALSE) => i is available */

/*************
 *
 *   next_available_multiplier()
 *
 *************/

static
int next_available_multiplier()
{
  int i;

  for (i = 0; i < MAX_MULTIPLIERS; i++)
    if (!Multipliers[i]) {
      Multipliers[i] = TRUE;
      return i;
    }
  fatal_error("next_available_multiplier, none avaiable (infinite loop?).");
  return -1;  /* to quiet compiler */
}  /* next_available_multiplier */

/*
 * memory management
 */

static unsigned Context_gets, Context_frees;
static unsigned Trail_gets, Trail_frees;

#define BYTES_CONTEXT sizeof(struct context)
#define PTRS_CONTEXT BYTES_CONTEXT%BPP == 0 ? BYTES_CONTEXT/BPP : BYTES_CONTEXT/BPP + 1

#define BYTES_TRAIL sizeof(struct trail)
#define PTRS_TRAIL BYTES_TRAIL%BPP == 0 ? BYTES_TRAIL/BPP : BYTES_TRAIL/BPP + 1

/*************
 *
 *   Context get_context()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
Context get_context(void)
{
  Context p = get_mem(PTRS_CONTEXT);
  p->multiplier = next_available_multiplier();
  Context_gets++;
  return(p);
}  /* get_context */

/*************
 *
 *    free_context()
 *
 *************/

/* DOCUMENTATION
*/

/* PUBLIC */
void free_context(Context p)
{
  Multipliers[p->multiplier] = FALSE;
  free_mem(p, PTRS_CONTEXT);
  Context_frees++;
}  /* free_context */

/*************
 *
 *   Trail get_trail()
 *
 *************/

static
Trail get_trail(void)
{
  Trail p = get_mem(PTRS_TRAIL);
  Trail_gets++;
  return(p);
}  /* get_trail */

/*************
 *
 *    free_trail()
 *
 *************/

static
void free_trail(Trail p)
{
  free_mem(p, PTRS_TRAIL);
  Trail_frees++;
}  /* free_trail */

/*************
 *
 *   fprint_unify_mem()
 *
 *************/

/* DOCUMENTATION
This routine prints (to FILE *fp) memory usage statistics for data types
associated with the unify package.
The Boolean argument heading tells whether to print a heading on the table.
*/

/* PUBLIC */
void fprint_unify_mem(FILE *fp, BOOL heading)
{
  int n;
  if (heading)
    fprintf(fp, "  type (bytes each)        gets      frees     in use      bytes\n");

  n = BYTES_CONTEXT;
  fprintf(fp, "context (%4d)      %11u%11u%11u%9.1f K\n",
          n, Context_gets, Context_frees,
          Context_gets - Context_frees,
          ((Context_gets - Context_frees) * n) / 1024.);

  n = BYTES_TRAIL;
  fprintf(fp, "trail (%4d)        %11u%11u%11u%9.1f K\n",
          n, Trail_gets, Trail_frees,
          Trail_gets - Trail_frees,
          ((Trail_gets - Trail_frees) * n) / 1024.);

}  /* fprint_unify_mem */

/*************
 *
 *   p_unify_mem()
 *
 *************/

/* DOCUMENTATION
This routine prints (to stdout) memory usage statistics for data types
associated with the unify package.
*/

/* PUBLIC */
void p_unify_mem()
{
  fprint_unify_mem(stdout, 1);
}  /* p_unify_mem */

/*
 *  end of memory management
 */
/*************
 *
 *   unify()
 *
 *************/

/* DOCUMENTATION
This routine tries to unify two terms in their respective
contexts.  Trail * trp is the address of a Trail.
If successful, the trail is extended (at its front) with
substitutions that were made, and trp is updated to point to
the new beginning of the trail.  If unify fails, the Contexts and
the Trail * are not changed.
<P>
You must make sure, before calling unify(), that no variable v in
t1 or t2 has VARNUM(v) >= MAXVARS.  This is usually accomplished by
calling a routine that renames variables.
<P>
Here is an example how to use unify(),
apply(), and undo_subst().  Assume we have terms t1 and t2.
(Terms t1 and t2 may share variables, but we "separate" the
variables by using different contexts.  That is, variable v1 in
context c1 is different from variable v1 in context c2.)
<PRE>
    {
        Context c1 = get_context();
        Context c2 = get_context();
        Trail tr = NULL;
        if (unify(t1, c1, t2, c2, &tr)) {
            Term t3 = apply(t1, c1);
            Term t4 = apply(t2, c2);
            if (term_ident(t3, t4))
                printf("everything is OK\n");
            else
                printf("something is broken\n");
            undo_subst(tr);
            zap_term(t3);
            zap_term(t4);
        }
        else
            printf("unify fails\n");
        free_context(c1);
        free_context(c2);
    }
</PRE>
*/

/* PUBLIC */
BOOL unify(Term t1, Context c1,
          Term t2, Context c2, Trail *trp)
{
  Trail tpos, tp, t3;
  int vn1, vn2;

  DEREFERENCE(t1, c1)  /* dereference macro */

  DEREFERENCE(t2, c2)  /* dereference macro */

  /* Now, neither t1 nor t2 is a bound variable. */

  if (VARIABLE(t1)) {
    vn1 = VARNUM(t1);
    if (VARIABLE(t2)) {
      /* both t1 and t2 are variables */
      if (vn1 == VARNUM(t2) && c1 == c2)
	return TRUE;  /* identical */
      else {
	BIND_TR(vn1, c1, t2, c2, trp)
	return TRUE;
      }
    }
    else {
      /* t1 variable, t2 not variable */
      if (occur_check(vn1, c1, t2, c2)) {
	BIND_TR(vn1, c1, t2, c2, trp)
	return TRUE;
      }
      else
	return FALSE;  /* failed occur_check */
    }
  }

  else if (VARIABLE(t2)) {
    /* t2 variable, t1 not variable */
    vn2 = VARNUM(t2);
    if (occur_check(vn2, c2, t1, c1)) {
      BIND_TR(vn2, c2, t1, c1, trp)
      return TRUE;
    }
    else
      return FALSE;  /* failed occur_check */
  }
    
  else if (SYMNUM(t1) != SYMNUM(t2))
    return FALSE;  /* fail because of symbol clash */

  else if (t1->arity == 0)
    return TRUE;

  else {  /* both complex with same symbol */
    int i, arity;

    tpos = *trp;  /* save trail position in case of failure */
	
    i = 0; arity = t1->arity;
    while (i < arity && unify(t1->args[i], c1, t2->args[i], c2, trp))
      i++;

    if (i == arity)
      return TRUE;
    else {  /* restore trail and fail */
      tp = *trp;
      while (tp != tpos) {
	tp->context->terms[tp->varnum] = NULL;
	tp->context->contexts[tp->varnum] = NULL;
	t3 = tp;
	tp = tp->next;
	free_trail(t3);
      }
      *trp = tpos;
      return FALSE;
    }
  }
}  /* unify */

/*************
 *
 *    int variant(t1, c1, t2, trail_address)
 *
 *************/

/* DOCUMENTATION
This routine checks if Term t1 (in Context c1) and Term t2
(without a Context) are variants, that is, if each is an instance of the other.
If successful, the unifying substitution is in Context c1.
The calling sequence and the use of Contexts and Trails is the same
as for unify().
*/

/* PUBLIC */
BOOL variant(Term t1, Context c1,
	    Term t2, Trail *trp)
{
  /* If this gets used a lot, it should be recoded so that it won't
   * traverse the terms twice.
   */
  BOOL ok;
  Trail tr = NULL;
  Context c2 = get_context();

  if (match(t2, c2, t1, &tr)) {
    undo_subst(tr);
    ok = match(t1, c1, t2, trp);
  }
  else
    ok = 0;

  free_context(c2);
  return ok;
}  /* variant */

/*************
 *
 *    int occur_check(varnum, var_context, term, term_context)
 *
 *    Return 0 iff variable occurs in term under substitution
 *       (including var==term).
 *
 *************/

/* DOCUMENTATION
This function checks if a variable with index vn (in Context vc)
occurs in Term t (in Context c), including the top case, where t
is the variable in question.
*/

/* PUBLIC */
BOOL occur_check(int vn, Context vc, Term t, Context c)
{
  if (!c)
    return TRUE;
  else if (VARIABLE(t)) {  /* variable */
    int tvn;
    tvn = VARNUM(t);
    if (tvn == vn && c == vc)
      return FALSE;  /* fail occur_check here */
    else if (c->terms[tvn] == NULL)
      return TRUE;  /* uninstantiated variable */
    else
      return occur_check(vn, vc, c->terms[tvn], c->contexts[tvn]);
  }
  else {  /* constant or complex */
    int i;
    for (i = 0; i < t->arity; i++)
      if (!occur_check(vn, vc, t->args[i], c))
	return FALSE;
    return TRUE;
  }
}  /* occur_check */

/*************
 *
 *    int match(t1, c1, t2, trail_address) -- one-way unification.
 *
 *        Match returns 1 if t2 is an instance of {t1 in context c1}.
 *    This is not a very general version, but it is useful for
 *    demodulation and subsumption.  It assumes that the variables
 *    of t1 and t2 are separate, that none of the variables in t2
 *    have been instantiated, and that none of those t2's variables
 *    will be instantiatied.  Hence, there is no context for t2,
 *    no need to dereference more than one level, and no need for
 *    an occur_check.
 *
 *        The use of the trail is the same as in `unify'.
 *
 *************/

/* DOCUMENTATION
This routine checks if Term t2 (without a Context) is an
instance of Term t1 (in Context c1).
If successful, Context c1 and Trail * trp are updated.
The calling sequence and the use of Contexts and Trails is similar
to those for unify().
*/

/* PUBLIC */
BOOL match(Term t1, Context c1, Term t2, Trail *trp)
{
  int vn;

  if (VARIABLE(t1)) {
    vn = VARNUM(t1);
    if (c1->terms[vn] == NULL) {
      BIND_TR(vn, c1, t2, NULL, trp)
      return TRUE;
    }
    else
      return term_ident(c1->terms[vn], t2);
  }
  else if (VARIABLE(t2))
    return FALSE;
  else {  /* neither term is a variable */
    if (SYMNUM(t1) != SYMNUM(t2))
      return FALSE;  /* fail because of symbol clash */
    else {
      Trail tpos, tp, t3;
      int i, arity;

      tpos = *trp;  /* save trail position in case of failure */
      i = 0; arity = t1->arity;
      while (i < arity && match(t1->args[i], c1, t2->args[i], trp))
	i++;
      if (i == arity)
	return TRUE;
      else {  /* restore from trail and fail */
	tp = *trp;
	while (tp != tpos) {
	  tp->context->terms[tp->varnum] = NULL;
	  t3 = tp;
	  tp = tp->next;
	  free_trail(t3);
	}
	*trp = tpos;
	return FALSE;
      }
    }
  }
}  /* match */

/*************
 *
 *    Term apply(term, context) -- Apply a substitution to a term.
 *
 *    Apply always succeeds and returns a pointer to the
 *    instantiated term.
 *
 *************/

/* DOCUMENTATION
This routine applies the substitution in Context c to Term t.
See the explanation of unify() for an example of the use of apply().
*/

/* PUBLIC */
Term apply(Term t, Context c)
{
  DEREFERENCE(t, c)

  /* A NULL context is ok.  It happens when c is built by match. */
  /* If the context is NULL, then apply just copies the term.    */
    
  if (VARIABLE(t)) {
    if (!c)
      return get_variable_term(VARNUM(t));
    else
      return get_variable_term(c->multiplier * MAX_VARS + VARNUM(t));
  }
  else {  /* constant or complex term */
    Term t2 = get_rigid_term_like(t);
    int i;
    for (i = 0; i < t->arity; i++)
      t2->args[i] = apply(t->args[i], c);
    return t2;
  }
}  /* apply */

/*************
 *
 *    apply_substitute()
 *
 *************/

/* DOCUMENTATION
This routine is like apply(), but when it reaches a particular subterm
(into_term) of the source term (t), it continues with another source
term (beta).
This routine is intended to be used for paramodulation, to avoid
unnecessary work.  For example, when paramodulating alpha=beta into
p[into_term], where alpha unifies with into_term, we construct
the appropriate instance of p[beta] in one step by using this routine.
*/

/* PUBLIC */
Term apply_substitute(Term t, Term beta, Context c_from,
		      Term into_term, Context c_into)
{
  if (t == into_term)
    return apply(beta, c_from);
  else if (VARIABLE(t))
    return apply(t, c_into);
  else {
    Term t2 = get_rigid_term_like(t);
    int i;
    for (i = 0; i < t->arity; i++)
      t2->args[i] = apply_substitute(t->args[i], beta, c_from,
				     into_term, c_into); 
    return t2;
  }
}  /* apply_substitute */

/*************
 *
 *    undo_subst(trail_1) -- Clear a substitution.
 *
 *************/

/* DOCUMENTATION
This routine clears substitution entries recoded in Trail tr,
and frees the corresponding Trail nodes.
*/

/* PUBLIC */
void undo_subst(Trail tr)
{
  Trail t3;
  while (tr != NULL) {
    tr->context->terms[tr->varnum] = NULL;
    tr->context->contexts[tr->varnum] = NULL;
    t3 = tr;
    tr = tr->next;
    free_trail(t3);
  }
}  /* undo_subst */

/*************
 *
 *    undo_subst_2(trail_1, trail_2) -- Clear part of a substitution.
 *
 *    It is assumed that trail_2 (possibly NULL) is a subtrail
 *    of trail_1. This routine clears entries starting at trail_1,
 *    up to (but not including) trail_2.
 *
 *************/

/* DOCUMENTATION
It is assumed that Trail sub_tr is a subtrail of Trail tr.
This routine clears part (maybe all) of a substitution, by
clearing the entries from tr up to, but not including sub_tr.
The corresponding Trail nodes are deallocated, so the
caller should no longer refer to tr.  (This is useful for
inference rules like hyperresolution, which backtrack,
undoing parts of substitutions.)
*/

/* PUBLIC */
void undo_subst_2(Trail tr, Trail sub_tr)
{
  Trail t3;
  while (tr != sub_tr) {
    tr->context->terms[tr->varnum] = NULL;
    tr->context->contexts[tr->varnum] = NULL;
    t3 = tr;
    tr = tr->next;
    free_trail(t3);
  }
}  /* undo_subst_2 */

/*************
 *
 *    fprint_context(file_ptr, context)
 *
 *************/

/* DOCUMENTATION
This routine prints (to FILE *fp) a Context.
*/

/* PUBLIC */
void fprint_context(FILE *fp, Context c)
{
  int i;
  
  if (c == NULL)
    fprintf(fp, "Substitution NULL.\n");
  else {
    fprintf(fp, "Substitution, multiplier %d\n", c->multiplier);
    for (i=0; i< MAX_VARS; i++) {
      if (c->terms[i] != NULL) {
	Term t = get_variable_term(i);
	fprint_term(fp, t);
	free_term(t);
	fprintf(fp, " [0x%x] -> ", (unsigned) c);
	fprint_term(fp, c->terms[i]);
	if (c->contexts[i] == NULL)
	  fprintf(fp, " (NULL context)\n");
	else
	  fprintf(fp, " [0x%x:%d]\n", (unsigned) c->contexts[i],
		  c->contexts[i]->multiplier);
      }
    }
#if 0    
    if (c->partial_term) {
      printf("partial_term: ");
      print_term(fp, c->partial_term);
      printf("\n");
    }
#endif
  }
}  /* fprint_context */

/*************
 *
 *    p_context(context)
 *
 *************/

/* DOCUMENTATION
This routine prints (to stdout) a Context.
*/

/* PUBLIC */
void p_context(Context c)
{
  fprint_context(stdout, c);
}  /* p_context */

/*************
 *
 *    fprint_trail(file_ptr, context)
 *
 *************/

/* DOCUMENTATION
This routine prints (to FILE *fp) a Trail.  The whole list is printed.
*/

/* PUBLIC */
void fprint_trail(FILE *fp, Trail t)
{
  Trail t2;
  fprintf(fp, "Trail:");
  t2 = t;
  while (t2 != NULL) {
    fprintf(fp, " <%d,%x>", t2->varnum, (unsigned) t2->context);
    t2 = t2->next;
  }
  fprintf(fp, ".\n");
}  /* fprint_trail */

/*************
 *
 *    p_trail(context)
 *
 *************/

/* DOCUMENTATION
This routine prints (to stdout) a Trail.  The whole list is printed.
*/

/* PUBLIC */
void p_trail(Trail t)
{
  fprint_trail(stdout, t);
}  /* p_trail */

/*************
 *
 *   match_weight2()
 *
 *************/

/* DOCUMENTATION
Special-purpose match for weighting.
*/

/* PUBLIC */
BOOL match_weight2(Term t1, Context c1, Term t2, Trail *trp, int var_sn)
{
  if (SYMNUM(t1) == var_sn) {
    return VARIABLE(t2);
  }
  else if (VARIABLE(t1)) {
    int vn = VARNUM(t1);
    if (c1->terms[vn] == NULL) {
      BIND_TR(vn, c1, t2, NULL, trp)
      return TRUE;
    }
    else
      return term_ident(c1->terms[vn], t2);
  }
  else if (VARIABLE(t2))
    return FALSE;
  else {  /* neither term is a variable */
    if (SYMNUM(t1) != SYMNUM(t2))
      return FALSE;  /* fail because of symbol clash */
    else {
      Trail tpos, tp, t3;
      int i, arity;

      tpos = *trp;  /* save trail position in case of failure */
      i = 0; arity = t1->arity;
      while (i < arity &&
	     match_weight2(t1->args[i], c1, t2->args[i], trp, var_sn))
	i++;
      if (i == arity)
	return TRUE;
      else {  /* restore from trail and fail */
	tp = *trp;
	while (tp != tpos) {
	  tp->context->terms[tp->varnum] = NULL;
	  t3 = tp;
	  tp = tp->next;
	  free_trail(t3);
	}
	*trp = tpos;
	return FALSE;
      }
    }
  }
}  /* match_weight2 */


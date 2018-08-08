// BV(2009-jun-18): Like multigoals, except
//     include positive form of proved goal on following input sos lists
//        (especially useful when proof was 2 way)
//     accumulate hints across runs

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

// ***********************************************************************
// MultiGoals
//
// Last updated:  2008-mar-09
// ***********************************************************************

#define PROVER_NAME     "MultiGoals"
#include "../VERSION_DATE.h"

#include "provers.h"  // includes LADR and search definitions

int MultiGoals(Prover_input input, Plist goals_list);

/*************
 *
 *    main -- MultiGoals
 *
 *************/

int main(int argc, char **argv)
{
  Prover_input input;
  int return_code;

  // new input list
  Plist goals_list;
  accept_list("goals_list",  FORMULAS,  FALSE, &goals_list);

  print_banner(argc, argv, PROVER_NAME, PROGRAM_VERSION, PROGRAM_DATE, FALSE);

  /***************** Initialize and read the input ***************************/

  input = std_prover_init_and_input(argc, argv,
			    TRUE,           // clausify
			    TRUE,           // echo input to stdout
			    KILL_UNKNOWN);  // unknown flags/parms are fatal

  // first processing of multigoals list, but not clausified yet
  goals_list = embed_formulas_in_topforms(goals_list, FALSE);

  fprintf(stdout,"\nList of goals:\n");
  fwrite_clause_list(stdout, goals_list, "multigoals", CL_FORM_STD);

  // ### It would be nice to recognize and disable an input goals list, but
  // ### I don't see a simple way to do it, since it will already be
  // ### processed (and the denials already included on sos).
  // ### One concern is that max_proofs not get bumped by auto_denials.

  printf("\n%% Warning:  Any regular input goals will be negated and included on sos.\n");

  clear_flag(input->options->auto_denials, TRUE);
  printf("\n%% clear(auto_denials), because it is incompatiable with multigoals iterations.\n");

  assign_parm(input->options->max_proofs, 1, TRUE);
  printf("\n%% Warning:  max_proofs set to 1 for multigoals iterations.\n");

  /***************** Search for a proof **************************************/

  return_code = MultiGoals(input, goals_list);

  /***************** Print result message and exit ***************************/

  // The return code is from the most recent execution of the prover.

  exit_with_message(stdout, return_code);
  exit(1);  // to satisfy the compiler (won't be called)
}  // main

int MultiGoals(Prover_input input, Plist goals_list)
{
   // save copy of input sos to reset after each goal iteration
   Plist original_sos = copy_plist(input->sos);

   // BV(2009-jun-18): proved goals
   Plist proved_goal = NULL;

   Plist denials;
   Topform next_goal;

   Prover_results results;
   int return_code = MAX_PROOFS_EXIT;
   int iteration = 0;
   int proof_count = 0;

   int goals_count = plist_count(goals_list);

   for (iteration = 0; iteration < goals_count; iteration++)
   {
      fprintf(stdout,"\n***********************************************\n");
      fprintf(stdout,"BEGIN GOAL %d\n", iteration+1);
      fprintf(stdout,"***********************************************\n");

      // get next multigoal (which may be a formula)
      next_goal = goals_list->v;
      goals_list = plist_pop(goals_list);

      fprintf(stdout,"\nAttempt to prove goal:  ");
      fwrite_clause(stdout, next_goal, CL_FORM_BARE);
      fprintf(stdout,"\n");

      // intialize regular input goals list with next multigoal
      zap_plist(input->goals);
      input->goals = NULL;
      input->goals = plist_append(input->goals, next_goal);

      // process regular goals list
      denials = process_goal_formulas(input->goals, TRUE);
      input->goals = NULL;

         // Note:  process_goal_formulas zaps (shallow) the input->goals
         // list, so it is not necessary to do it here.
         //
         // ### In fact, zapping the zapped list causes a SIGSEGV.

      fprintf(stdout,"\nDenial clauses:\n");
      fwrite_clause_list(stdout, denials, "denials", CL_FORM_STD);

      // reset sos for new problem
      zap_plist(input->sos);
      input->sos = copy_plist(original_sos);
      input->sos = plist_cat(input->sos, denials);

      // execute prover
      results = forking_search(input);
      return_code = results->return_code;

      if (return_code == MAX_PROOFS_EXIT)
      {
         // BV(2009-jun-18): append proved goal to "original" input sos
         //     (needs to be clausified)
         unassign_clause_id(next_goal);
         if (proved_goal != NULL)
            zap_plist(proved_goal);
         proved_goal = NULL;
         proved_goal = plist_append(proved_goal, next_goal);
         proved_goal = process_input_formulas(proved_goal, FALSE);

         fprintf(stdout,"\nAccept previous proved goal:\n");
         fwrite_clause_list(stdout, proved_goal, "proved_goal", CL_FORM_STD);

         original_sos = plist_append(original_sos, proved_goal->v);

         proof_count++ ;
      }
   }

   printf("\nMultigoals summary:  %d of %d goals proved\n", proof_count, iteration);

   // return code from most recent execution of prover
   return return_code;

} // MultiGoals

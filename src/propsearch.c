#include "propsearch.h"
#include "fastassign.h"
#include "print.h"
#include "trail.h"
#include "bump.h"
#define PROPAGATE_LITERAL search_propagate_literal
#define PROPAGATION_TYPE "search"

#include "proplit.h"

static inline void
update_search_propagation_statistics (kissat *solver,
                                      const unsigned *saved_propagate) {
  assert (saved_propagate <= solver->propagate);
  const unsigned propagated = solver->propagate - saved_propagate;

  LOG ("propagated %u literals", propagated);
  LOG ("propagation took %" PRIu64 " ticks", solver->ticks);

  ADD (propagations, propagated);
  ADD (ticks, solver->ticks);

  ADD (search_propagations, propagated);
  ADD (search_ticks, solver->ticks);

  if (solver->stable) {
    ADD (stable_propagations, propagated);
    ADD (stable_ticks, solver->ticks);
  } else {
    ADD (focused_propagations, propagated);
    ADD (focused_ticks, solver->ticks);
  }
}

static clause *search_propagate (kissat *solver) {
  clause *res = 0;
  unsigned *propagate = solver->propagate;
  while (!res && propagate != END_ARRAY (solver->trail))
    res = search_propagate_literal (solver, *propagate++);
  solver->propagate = propagate;
  return res;
}

clause *kissat_search_propagate (kissat *solver) {
  assert (!solver->probing);
  assert (solver->watching);
  assert (!solver->inconsistent);

  START (propagate);

  solver->ticks = 0;
  const unsigned *saved_propagate = solver->propagate;
  clause *conflict = search_propagate (solver);
  update_search_propagation_statistics (solver, saved_propagate);
  kissat_update_conflicts_and_trail (solver, conflict, true);
  if (conflict && solver->randec) {
    if (!--solver->randec)
      kissat_very_verbose (solver, "last random decision conflict");
    else if (solver->randec == 1)
      kissat_very_verbose (solver,
                           "one more random decision conflict to go");
    else
      kissat_very_verbose (solver,
                           "%s more random decision conflicts to go",
                           FORMAT_COUNT (solver->randec));
  }

  STOP (propagate);


  // CHB
  if(solver->stable && solver->heuristic==1){
    int i = SIZE_STACK (solver->trail) - 1;
    unsigned lit = i>=0?PEEK_STACK (solver->trail, i):0;  
    while(i>=0 && LEVEL(lit)==solver->level){
      lit = PEEK_STACK (solver->trail, i);
      kissat_bump_chb(solver,IDX(lit), conflict? 1.0 : 0.9); 
      i--;	    
    }
  }  
  if(solver->stable && solver->heuristic==1 && conflict) kissat_decay_chb(solver);
  return conflict;
}

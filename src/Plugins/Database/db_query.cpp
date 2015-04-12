
/******************************************************************************
* MODULE     : db_query.cpp
* DESCRIPTION: TeXmacs database queries
* COPYRIGHT  : (C) 2015  Joris van der Hoeven
*******************************************************************************
* This software falls under the GNU general public license version 3 or later.
* It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
* in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
******************************************************************************/

#include "Database/database.hpp"
#include "analyze.hpp"

/******************************************************************************
* Fast filtering of lines which satisfy a list of constraints
******************************************************************************/

bool
database_rep::line_satisfies (db_line_nr nr, db_constraints cs, db_time t) {
  db_line& l= db[nr];
  //cout << "  Testing " << l->id << ", " << l->attr << ", " << l->val << LF;
  if (t < l->created || t >= l->expires) return false;
  for (int i=0; i<N(cs); i++) {
    db_constraint c= cs[i];
    db_atom attr= c[0];
    if (l->attr != attr && attr != -1) return false;
    bool ok= false;
    for (int j=1; j<N(c); j++)
      ok= ok || l->val == c[j];
    if (!ok) return false;
  }
  return true;
}

bool
database_rep::id_satisfies (db_atom id, db_constraints cs, db_time t) {
  db_line_nrs nrs= id_lines[id];
  //cout << "Test " << id << ", " << nrs << LF;
  for (int i=0; i<N(nrs); i++)
    if (line_satisfies (nrs[i], cs, t)) return true;
  return false;
}

db_constraint
database_rep::encode_constraint (tree q) {
  db_constraint r;
  if (!is_tuple (q)) return db_constraint ();
  if (N(q) <= 1 || !is_atomic (q[0])) return db_constraint ();
  string attr= q[0]->label;
  if (attr == "any")
    r << -1;
  else if (atom_encode->contains (scm_unquote (q[0]->label)))
    r << atom_encode [scm_unquote (q[0]->label)];
  else return db_constraint ();
  for (int i=1; i<N(q); i++)
    if (atom_encode->contains (scm_unquote (q[i]->label)))
      r << atom_encode [scm_unquote (q[i]->label)];
  if (N(r) == 1) return db_constraint ();
  return r;
}

db_constraints
database_rep::encode_constraints (tree ql) {
  db_constraints r;
  bool failed= false;
  if (!is_tuple (ql)) failed= true;
  else for (int i=0; i<N(ql); i++) {
    db_constraint c= encode_constraint (ql[i]);
    if (N(c) == 0) failed= true;
    r << c;
  }
  if (failed) {
    r= db_constraints ();
    r << db_constraint ();
  }
  return r;
}

db_atoms
database_rep::filter (db_atoms ids, tree qt, db_time t, int limit) {
  //cout << "Query " << qt << "\n";
  db_constraints cs= encode_constraints (qt);
  //cout << "Encoded as " << q << "\n";
  if (N(cs) == 1 && N(cs) == 0) return db_atoms ();
  db_atoms r;
  for (int i=0; i<N(ids); i++)
    if (id_satisfies (ids[i], cs, t)) {
      r << ids[i];
      if (N(r) >= limit) break;
    }
  return r;
}

/******************************************************************************
* Estimating the complexity of a query
******************************************************************************/

int
database_rep::compute_complexity (tree q) {
  //cout << "Computing complexity of " << q << LF;
  if (!is_tuple (q)) return 0;
  if (N(q) <= 1 || !is_atomic (q[0])) return 0;
  if (!atom_encode->contains (scm_unquote (q[0]->label))) return 0;
  //db_atom attr= atom_encode [scm_unquote (q[0]->label)];
  int r=0;
  for (int i=1; i<N(q); i++)
    if (atom_encode->contains (scm_unquote (q[i]->label))) {
      db_atom val= atom_encode [scm_unquote (q[i]->label)];
      r += N (val_lines[val]);
    }
  //cout << "Return " << r << LF;
  return r;
}

int
database_rep::ansatz_index (tree q) {
  if (!is_tuple (q)) return -1;
  int best_i= -1;
  int best_c= 1000000000;
  for (int i=0; i<N(q); i++) {
    int c= compute_complexity (q[i]);
    if (c < best_c) {
      best_i= i;
      best_c= c;
    }
  }
  return best_i;
}

db_atoms
database_rep::ansatz (tree ql, db_time t) {
  if (!is_tuple (ql)) return db_atoms ();
  int a= ansatz_index (ql);
  //cout << "ansatz index: " << a << LF;
  if (a < 0) return ids_list;
  tree q= ql[a];
  db_atoms idsl;
  hashset<db_atom> idss;
  //db_atom attr= atom_encode [scm_unquote (q[0]->label)];
  for (int i=1; i<N(q); i++)
    if (atom_encode->contains (scm_unquote (q[i]->label))) {
      db_atom val= atom_encode [scm_unquote (q[i]->label)];
      db_line_nrs nrs= val_lines[val];
      //cout << "trying " << val << ", " << nrs << LF;
      for (int j=0; j<N(nrs); j++) {
        db_line& l= db[nrs[j]];
        //cout << "  line " << l->id << ", " << l->attr << ", " << l->val << LF;
        if ((t == 0) || (l->created <= t && t < l->expires))
          if (!idss->contains (l->id)) {
            idss->insert (l->id);
            idsl << l->id;
          }
      }
    }
  return idsl;
}

/******************************************************************************
* Master query routine
******************************************************************************/

db_atoms
database_rep::query (tree ql, db_time t, int limit) {
  //cout << "query " << ql << ", " << t << ", " << limit << LF;
  db_atoms ids= ansatz (ql, t);
  //cout << "ids= " << ids << LF;
  return filter (ids, ql, t, limit);
}
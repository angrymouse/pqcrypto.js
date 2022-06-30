/** 
 * \file rsr_algorithm.cpp
 * \brief Implementation of rsr_algorithm.h
 */

#include "rsr_algorithm.h"
#include "ffi.h"
#include "ffi_elt.h"
#include "ffi_vec.h"



/** 
 * \fn static void expand_space(ffi_vec &S, unsigned int &S_dim, unsigned int E_expected_dim, ffi_vec F, unsigned int F_dim)
 * \brief This function expands the space S
 *
 * \param[out] S Subspace generated by the coordinates of the syndrom
 * \param[out] S_dim Dimension of the space <b>S</b>
 * \param[in] E_expected_dim Expected dimension of the support <b>E</b>
 * \param[in] F Support of the vectors <b>x</b> and <b>y</b>
 * \param[in] F_dim Dimension of the support <b>F</b>
 */
static void expand_space(ffi_vec &S, unsigned int &S_dim, unsigned int E_expected_dim, ffi_vec F,  unsigned int F_dim) {
  // Compute the spaces Si = F[i]^(-1) * S
  ffi_vec Si[F_dim];
  ffi_elt Fi_inv;
  unsigned int Si_dim = S_dim;
  for(unsigned int i = 0 ; i < F_dim ; ++i) {
    ffi_elt_inv(Fi_inv, F[i]);
    ffi_vec_scalar_mul(Si[i], S, Fi_inv, Si_dim);
  }

  // Compute the spaces inters[i] as the intersection between Si and Si+1
  ffi_vec inters[F_dim-1];
  unsigned int inters_dim[F_dim-1];
  for(unsigned int i = 0 ; i < F_dim - 1 ; ++i) {
    ffi_vec_intersection(inters[i], inters_dim[i], Si[i], Si_dim, Si[i+1], Si_dim);
  }

  // Expand S until its dimension is F_dim * E_expected_dim
  ffi_vec inter, dsum1, dsum2, add, tmp1, tmp2;
  unsigned int inter_dim, dsum2_dim, tmp1_dim, tmp2_dim;
  for(unsigned int i = 0 ; i < F_dim - 2 ; ++i) {
    ffi_vec_intersection(inter, inter_dim, Si[i], Si_dim, Si[i+2], Si_dim);

    ffi_vec_directsum(dsum1, inters[i], inters_dim[i], inters[i+1], inters_dim[i+1]);
    ffi_vec_directsum(dsum2, dsum1, inters_dim[i] + inters_dim[i+1], inter, inter_dim);
    dsum2_dim = ffi_vec_gauss(dsum2, inters_dim[i] + inters_dim[i+1] + inter_dim);

    ffi_vec_tensor_mul(add, F, F_dim, dsum2, dsum2_dim);
    ffi_vec_directsum(tmp1, S, S_dim, add, F_dim * dsum2_dim);
    tmp1_dim = ffi_vec_gauss(tmp1, S_dim + F_dim * dsum2_dim);

    if(tmp1_dim > F_dim * E_expected_dim) continue;
    else {
      ffi_vec_directsum(tmp2, S, S_dim, tmp1, tmp1_dim);
      tmp2_dim = ffi_vec_gauss(tmp2, S_dim + tmp1_dim);
      ffi_vec_set(S, tmp2, tmp2_dim);
      S_dim = tmp2_dim;
    }
  }
}



/** 
 * \fn void rank_support_recover(ffi_vec& E, unsigned int& E_dim, unsigned int E_expected_dim, const ffi_vec& F, unsigned int F_dim, const ffi_vec& ec, unsigned int ec_size)
 * \brief This function implements the rank_support_recover using an improved version of the general decoding algorithm of the LRPC codes.
 *
 * As explained in the supporting documentation, the provided algorithm works as follows:
 *   1. Compute the vector space <b>S</b> generated by the coordinates of <b>ec</b> ;
 *   2. Retrieve the support <b>E</b> by computing the intersection of the spaces <b>Si</b> = <b>F[i]^(-1)</b> * <b>S</b>.
 *
 * The second part of the algorithm will fail if and only if <b>S</b> is not equal to <b>E.F</b>.
 * The DFR associated to each parameter set is given in the supporting documentation.
 *
 * \param[out] E Support of <b>r1</b> and <b>r2</b>
 * \param[in] E_dim Dimension of the support <b>E</b>
 * \param[in] E_expected_dim Expected dimension of the support <b>E</b>
 * \param[in] F Support of the vectors <b>x</b> and <b>y</b>
 * \param[in] F_dim Dimension of the support <b>F</b>
 * \param[in] ec Vector <b>ec</b> = <b>x</b>.<b>r2</b> - <b>y</b>.<b>r1</b> + <b>er</b>
 * \param[in] ec_size Size of the vector <b>ec</b>
 */
void rank_support_recover(ffi_vec& E, unsigned int& E_dim, unsigned int E_expected_dim, const ffi_vec& F, unsigned int F_dim, const ffi_vec& ec, unsigned int ec_size) {
  
  /*
   * Step 1: Compute the vector space S 
   */ 

  // Compute the vector space S from the coordinates of ec (line 1)
  ffi_vec S;
  ffi_vec_set(S, ec, ec_size);
  unsigned int S_dim = ffi_vec_gauss(S, ec_size);

  #ifdef VERBOSE
    printf("\n\n# Rank Support Recover - Begin #");
    printf("\n\nS: "); ffi_vec_print(S, S_dim);
  #endif

  // Expand the vector space S (lines 2 to 9)
  expand_space(S, S_dim, E_expected_dim, F, F_dim);


  /*
   * Step 2: Recover the vector space E 
   */ 

  // Compute the spaces Si = F[i]^(-1) * S (line 10)
  ffi_vec Si[F_dim];
  ffi_elt Fi_inv;
  for(unsigned int i = 0 ; i < F_dim ; ++i) {
    ffi_elt_inv(Fi_inv, F[i]);
    ffi_vec_scalar_mul(Si[i], S, Fi_inv, S_dim);
  }

  // Retrieve the support E by computing the intersection of the Si spaces (line 10)
  ffi_vec_intersection(E, E_dim, Si[0], S_dim, Si[1], S_dim);

  if(E_dim > E_expected_dim) {
    for(unsigned int i = 2 ; i < F_dim ; ++i) {
      ffi_vec_intersection(E, E_dim, E, E_dim, Si[i], S_dim); 
      if(E_dim <= E_expected_dim) break;
    }
  }

  #ifdef VERBOSE
    printf("\n\nS: "); ffi_vec_print(S, S_dim);
    for(unsigned int i = 0 ; i < F_dim ; ++i) {
      printf("\n\nS%i: ", i); ffi_vec_print(Si[i], S_dim);
    }
    printf("\n\nE: "); ffi_vec_print(E, E_dim);
  #endif

  ffi_vec_echelonize(E, E_dim);

  #ifdef VERBOSE
    printf("\n\nE: "); ffi_vec_print(E, E_dim);
    printf("\n\n# Rank Support Recover - End #");
  #endif
}

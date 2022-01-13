#ifndef __GWASITER_H_
#define __GWASITER_H_

#include "mpc.h"
#include "assert.h"
#include <vector>
#include <NTL/mat_ZZ_p.h>
#include "libsodium_rng.cpp"

using namespace std;

class GwasIterator {
public:
  static const int TERM_CODE = 0;
  static const int G_CODE = 1;
  static const int GP_CODE = 2;
  static const int GM_CODE = 3;
  static const int GMP_CODE = 4;

  explicit GwasIterator(MPCEnv& mpc, RandomNumberGenerator& rng, int party_id) {
    this->mpc = &mpc;
    this->rng = &rng;
    this->party_id = party_id;
  }

  void Init(bool pheno_flag, bool missing_flag) {
    this->pheno_flag = pheno_flag;
    this->missing_flag = missing_flag;

    index = 0;

    if (party_id == 0) {
      num_left = Param::NUM_INDS;
    } else if (party_id == 1) {
      num_left = Param::NUM_INDS_SP_1;
    } else if (party_id == 2) {
      num_left = Param::NUM_INDS_SP_2;
    } else {
      assert(false);
    }

    cout << "Initialized GwasIterator" << endl;
  }

  int TransferMode() {
    if (missing_flag) {
      if (pheno_flag) {
        return GMP_CODE;
      } else {
        return GM_CODE;
      }
    } else {
      if (pheno_flag) {
        return GP_CODE;
      } else {
        return G_CODE;
      }
    }
  }

  bool NotDone() {
    return num_left > 0;
  }

  /* Return genotypes in dosage format (0, 1, or 2), assume no missing */
  void GetNextGP(Vec<ZZ_p>& g, Vec<ZZ_p>& p) {
    assert(TransferMode() == GP_CODE);
    Vec<ZZ_p> m;
    Mat<ZZ_p> gmat;
    GetNextAux(gmat, m, p);
    g = gmat[0];
  }
  void GetNextG(Vec<ZZ_p>& g) {
    assert(TransferMode() == G_CODE);
    Vec<ZZ_p> p;
    Vec<ZZ_p> m;
    Mat<ZZ_p> gmat;
    GetNextAux(gmat, m, p);
    g = gmat[0];
  }

  /* Return genotype probabilities with missingness information */
  void GetNextGM(Mat<ZZ_p>& g, Vec<ZZ_p>& m) {
    assert(TransferMode() == GM_CODE);
    Vec<ZZ_p> p;
    GetNextAux(g, m, p);
  }
  void GetNextGMP(Mat<ZZ_p>& g, Vec<ZZ_p>& m, Vec<ZZ_p>& p) {
    assert(TransferMode() == GMP_CODE);
    GetNextAux(g, m, p);
  }

private:
  MPCEnv *mpc;
  RandomNumberGenerator *rng;
  int party_id;
  int num_left;
  int index;
  bool pheno_flag;
  bool missing_flag;

  void GetNextAux(Mat<ZZ_p>& g, Vec<ZZ_p>& m, Vec<ZZ_p>& p) {
    assert(NotDone());

    if (missing_flag) {
      g.SetDims(3, Param::NUM_SNPS);
      m.SetLength(Param::NUM_SNPS);
    } else {
      g.SetDims(1, Param::NUM_SNPS);
    }

    if (party_id == 1 || party_id == 2) {
      if (pheno_flag) {
        rng->RandVec(p, 1 + Param::NUM_COVS);
      }
      if (missing_flag) {
        rng->RandMat(g, 3, Param::NUM_SNPS);
        rng->RandVec(m, Param::NUM_SNPS);
      } else {
        rng->RandMat(g, 1, Param::NUM_SNPS);
      }    
    }

    index++;
    num_left--;
  }
};

#endif

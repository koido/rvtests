#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#include "LinearRegressionScoreTest.h"

#include "third/eigen/Eigen/Cholesky"
#include "third/gsl/include/gsl/gsl_cdf.h"  // use gsl_cdf_chisq_Q

#include "regression/MatrixOperation.h"

LinearRegressionScoreTest::LinearRegressionScoreTest()
    : pvalue(0.0), stat(0.0){};

bool LinearRegressionScoreTest::FitLinearModel(Matrix& X, Vector& y,
                                               int colToTest) {
  Matrix Xnull;
  Vector Xcol;
  this->splitMatrix(X, colToTest, Xnull,
                    Xcol);  // Xnull is the X matrix after taking out xcol
  // LinearRegression lr;
  // if (this->lr.FitLinearModel(Xnull, y, nRound) == false){
  //     return false;
  // }
  if (!this->FitNullModel(Xnull, y)) return false;
  if (!this->TestCovariate(Xnull, y, Xcol)) return false;
  return true;
}

bool LinearRegressionScoreTest::FitNullModel(Matrix& Xnull, Vector& y) {
  if (this->lr.FitLinearModel(Xnull, y) == false) {
    return false;
  }
  return true;
}

bool LinearRegressionScoreTest::TestCovariate(Matrix& Xnull, Vector& y,
                                              Vector& Xcol) {
  this->Umatrix.Dimension(1, 1);
  this->Vmatrix.Dimension(1, 1);
  this->betaMatrix.Dimension(1, 1);

  double& U = Umatrix(0, 0);
  double& I = Vmatrix(0, 0);

  U = 0.0;
  I = 0.0;

  // printf("size of betaHat = %d\n",betaHat1.Length());

  // define a vector and a matrix for I_r = V_rr - corr*(mat_corr)^{-1}*corr
  // int nParamRemain = X.cols - 1;
  // int nParamRemain = Xnull.cols;
  // Vector vec_corr;
  // vec_corr.Dimension(nParamRemain, 0.0);
  // Matrix mat_corr;
  // mat_corr.Dimension(nParamRemain, nParamRemain, 0.0);

  // for (int i = 0; i < Xnull.rows; i++) {
  //   U += Xcol[i] * (y[i] - lr.GetPredicted()[i]);
  //   // calcualte vec_corr
  //   for (int j = 0; j < Xnull.cols; j++) {
  //     vec_corr[j] += Xcol[i] * Xnull(i, j);
  //     // printf("j = %d, xcol = %d, Xnull = %d\n",j,bnull, xcol[j],
  //     // Xnull(i,j));
  //   }

  //   // calcualte mat_corr
  //   for (int j = 0; j < nParamRemain; j++) {
  //     for (int k = 0; k < nParamRemain; k++) {
  //       mat_corr(j, k) += Xnull(i, j) * Xnull(i, k);
  //     }
  //   }
  //   I += Xcol[i] * Xcol[i];
  // }
  DECLARE_EIGEN_CONST_MATRIX(Xnull, Xnull_e);
  DECLARE_EIGEN_CONST_VECTOR(Xcol, Xcol_e);
  DECLARE_EIGEN_CONST_VECTOR(y, y_e);
  DECLARE_EIGEN_CONST_VECTOR(lr.GetPredicted(), pred_e);
  DECLARE_EIGEN_MATRIX(Umatrix, U_e);
  DECLARE_EIGEN_MATRIX(Vmatrix, V_e);
  U_e = Xcol_e.transpose() * (y_e - pred_e);

  // // inverse the mat_corr
  // // SVD svd;
  // // svd.InvertInPlace(mat_corr);
  // DECLARE_EIGEN_MATRIX(mat_corr, mat_corr_e);
  // mat_corr_e = mat_corr_e.llt().solve(
  //     Eigen::MatrixXd::Identity(mat_corr_e.rows(), mat_corr_e.cols()));

  // Vector leftMult_corr;
  // leftMult_corr.Dimension(nParamRemain, 0.0);

  // // updates I by substracting the terms led by correlations
  // // multiplying vec_corr with mat_corr
  // for (int i = 0; i < nParamRemain; i++) {
  //   for (int j = 0; j < nParamRemain; j++) {
  //     leftMult_corr[i] += vec_corr[j] * mat_corr(i, j);
  //   }
  // }

  // // multiplying mat_corr with vec_corr
  // for (int i = 0; i < nParamRemain; i++) {
  //   I -= leftMult_corr[i] * vec_corr[i];
  // }
  Eigen::MatrixXd XZ = Xcol_e.transpose() * Xnull_e;
  V_e = Xcol_e.transpose() * Xcol_e -
        XZ * (Xnull_e.transpose() * Xnull_e).llt().solve(XZ.transpose());

  this->betaMatrix(0, 0) = U / I;
  I *= this->lr.GetSigma2();

  // printf("In the end, I = %.5f\n",I);
  if (I < 1e-6) {
    this->pvalue = 0.0;
    return false;
  }

  this->stat = U * U / I;
  if (this->stat < 0) return false;
  // this->pvalue = chidist(this->stat, 1.0); // use chisq to inverse
  this->pvalue = gsl_cdf_chisq_Q(this->stat, 1.0);
  return true;
}

bool LinearRegressionScoreTest::TestCovariate(Vector& x, Vector& y) {
  this->Umatrix.Dimension(1, 1);
  this->Vmatrix.Dimension(1, 1);
  this->betaMatrix.Dimension(1, 1);

  double& U = Umatrix(0, 0);
  double& V = Vmatrix(0, 0);
  U = 0.0;
  V = 0.0;

  // notation is from Danyu Lin's paper
  double sumSi = 0.0;
  double sumSi2 = 0.0;
  double sumYi = 0.0;
  double sumYi2 = 0.0;
  int n = x.Length();
  for (int i = 0; i < n; i++) {
    sumSi += x[i];
    sumSi2 += x[i] * x[i];
    sumYi += y[i];
    sumYi2 += y[i] * y[i];
  };
  double yMean = sumYi / n;
  double sigma2 = (sumYi2 - sumYi * sumYi / n) / n;
  for (int i = 0; i < n; ++i) {
    U += (y[i] - yMean) * x[i];
  }

  V = (sumSi2 - sumSi / n * sumSi);
  this->betaMatrix(0, 0) = U / V;
  V *= sigma2;

  if (V < 1e-6) {
    this->pvalue = 0.0;
    return false;
  }
  this->stat = U * U / V;
  if (this->stat < 0) return false;
  this->pvalue = gsl_cdf_chisq_Q(this->stat, 1.0);
  return true;
};

/** NOTE:
 * S_i is column i of the transposed @param Xcol, S_i is m by 1 dimension
 * U = \sum_i (Y_i - \hat{\gamma}^T Z_i ) * S_i
 * V = \hat{\sigma}^2 ( \sum _i S_i S_i^T - (\sum Z_i S_i^T) T  inv(\sum Z_i
 * Z_i^T) (\sum Z_i S_i^T)
 * U^T*inv(V)*U is the score test statistic
 * Hypothese: test efficients of Xcol are all Zero
 */
bool LinearRegressionScoreTest::TestCovariate(const Matrix& Xnull,
                                              const Vector& y,
                                              const Matrix& Xcol) {
  if (Xnull.rows != y.Length() || y.Length() != Xcol.rows) {
    fprintf(stderr, "Incompatible dimension.\n");
    return false;
  }
  if (Xcol.cols == 0) return false;
  // int n = Xcol.rows;
  int m = Xcol.cols;
  int d = Xnull.cols;

  // Vector U(m);
  // Matrix SS(m, m);
  // Matrix SZ(m, d);
  // Matrix ZZ(d, d);
  // U.Zero();
  // SS.Zero();
  // SZ.Zero();
  // ZZ.Zero();

  // for (int i = 0; i < n; i++) {
  //   U.AddMultiple(this->lr.GetResiduals()[i], Xcol[i]);

  //   MatrixPlusEqualV1andV1T(SS, Xcol[i]);
  //   MatrixPlusEqualV1andV2T(SZ, Xcol[i], Xnull[i]);
  //   MatrixPlusEqualV1andV1T(ZZ, Xnull[i]);
  // }
  this->Umatrix.Dimension(m, 1);
  DECLARE_EIGEN_MATRIX(this->Umatrix, U);
  DECLARE_EIGEN_CONST_MATRIX(Xcol, Xcol_e);
  DECLARE_EIGEN_CONST_MATRIX(Xnull, Xnull_e);
  DECLARE_EIGEN_VECTOR(this->lr.GetResiduals(), resid_e);
  Eigen::MatrixXd SS(m, m);
  Eigen::MatrixXd SZ(m, d);
  Eigen::MatrixXd ZZ(d, d);
  U = Xcol_e.transpose() * resid_e;    // U [m x 1]
  SS = Xcol_e.transpose() * Xcol_e;    // SS [m x m]
  SZ = Xcol_e.transpose() * Xnull_e;   // SZ [m x d]
  ZZ = Xnull_e.transpose() * Xnull_e;  // ZZ [d x d]

  // // inverse in place ZZ
  // SVD svd;
  // svd.InvertInPlace(ZZ);

  // // Z = - SZ * (ZZ^-1) * ZS
  // Matrix ZS;
  // ZS.Transpose(SZ);
  // Matrix tmp;
  // tmp.Product(SZ, ZZ);
  // SZ.Product(tmp, ZS);
  // SZ.Negate();
  // SS.Add(SZ);

  SS -= SZ * ZZ.llt().solve(Eigen::MatrixXd::Identity(d, d)) * SZ.transpose();
  // copy(U, &this->Umatrix);

  // this->Vmatrix = SS;
  // this->Vmatrix *= lr.GetSigma2();
  this->Vmatrix.Dimension(m, m);
  DECLARE_EIGEN_MATRIX(this->Vmatrix, V_e);
  V_e = SS * lr.GetSigma2();

  // svd.InvertInPlace(SS);
  // Matrix Umat;
  // copy(U, &Umat);
  // this->betaMatrix.Product(SS, Umat);
  // SS /= lr.GetSigma2();
  this->betaMatrix.Dimension(m, 1);
  DECLARE_EIGEN_MATRIX(this->betaMatrix, beta_e);
  SS = SS.llt().solve(Eigen::MatrixXd::Identity(m, m));
  beta_e = SS * U;
  SS /= lr.GetSigma2();

  // // S = U^T inv(I) U : quadratic form
  // double S = 0.0;
  // for (int i = 0; i < m; i++) {
  //   S += U[i] * SS(i,i) * U[i];
  //   for (int j = i + 1; j < m; j++) {
  //     S += 2.0 * U[i] * SS(i,j) * U[j];
  //   }
  // }
  double S = (U.transpose() * SS * U).sum();

  this->stat = S;
  if (this->stat < 0) return false;
  this->pvalue = gsl_cdf_chisq_Q(
      this->stat,
      1.0);  // use chisq to inverse, here chidist = P(X > S) where X ~ chi(m)
  return true;
};

/** NOTE
 * S_i is column i of the transposed @param X, S_i is m by 1 dimension
 * U = \sum_i (Y_i - \hat{\gamma}^T Z_i ) * S_i
 * V = \hat{\sigma}^2 ( \sum _i S_i S_i^T - (\sum S_i)  (\sum S_i^T) / n)
 * U^T*inv(V)*U is the score test statistic
 */
bool LinearRegressionScoreTest::TestCovariate(const Matrix& X,
                                              const Vector& y) {
  if (X.rows != y.Length()) {
    fprintf(stderr, "Incompatible dimensino.\n");
    return false;
  }
  int m = X.cols;  // also: df
  int n = X.rows;

  // Vector U(m);
  // Matrix SS(m, m);
  // Vector SumS(m);
  // U.Zero();
  // SS.Zero();
  // SumS.Zero();
  this->Umatrix.Dimension(m, 1);
  DECLARE_EIGEN_VECTOR(this->Umatrix, U);
  Eigen::MatrixXd SS(m, m);
  Eigen::VectorXd SumS(m);

  // double yMean = y.Average();
  // for (int i = 0; i < X.rows; i++) {
  //   U.AddMultiple(y[i] - yMean, X[i]);
  //   MatrixPlusEqualV1andV2T(SS, X[i], X[i]);
  //   SumS.Add(X[i]);
  // }
  DECLARE_EIGEN_CONST_MATRIX(X, X_e);
  DECLARE_EIGEN_CONST_VECTOR(y, y_e);
  double yMean = y_e.sum() / y_e.size();
  U = X_e.transpose() * (y_e.array() - yMean).matrix();
  SS = X_e.transpose() * X_e;
  SumS = X_e.colwise().sum();  // SumS [1 x m]

  // Matrix temp(SS.rows, SS.cols);
  // temp.Zero();
  // MatrixPlusEqualV1andV2T(temp, SumS, SumS);
  // temp.Multiply(1.0 / n);
  // temp.Negate();
  // SS.Add(temp);
  SS -= SumS.transpose() * SumS / n;

  // copy(U, &this->Umatrix);

  // this->Vmatrix = SS;
  // this->Vmatrix *= lr.GetSigma2();
  // SVD svd;
  // svd.InvertInPlace(SS);
  // Matrix Umat;
  // copy(U, &Umat);
  // this->betaMatrix.Product(SS, Umat);
  // SS /= lr.GetSigma2();
  this->Vmatrix.Dimension(m, m);
  this->betaMatrix.Dimension(m, 1);
  DECLARE_EIGEN_MATRIX(this->Vmatrix, V_e);
  DECLARE_EIGEN_MATRIX(this->betaMatrix, beta_e);
  V_e = SS * lr.GetSigma2();
  Eigen::MatrixXd SSinv = SS.llt().solve(Eigen::MatrixXd::Identity(m, m));
  beta_e = SSinv * U;

  // double S = 0.0;
  // for (int i = 0; i < m; i++) {
  //   S += U[i] * SS(i,i) * U[i];
  //   for (int j = i + 1; j < m; j++) {
  //     S += 2.0 * U[i] * SS(i,j) * U[j];
  //   }
  // }

  // this->stat = S;
  this->stat = (U.transpose() * SSinv * U).sum() / lr.GetSigma2();
  if (this->stat < 0) return false;
  this->pvalue = gsl_cdf_chisq_Q(this->stat, 1.0);
  return true;
}

void LinearRegressionScoreTest::splitMatrix(Matrix& x, int col, Matrix& xnull,
                                            Vector& xcol) {
  if (x.cols < 2) {
    printf("input matrix has too few cols!\n");
  }
  xnull.Dimension(x.rows, x.cols - 1);
  xcol.Dimension(x.rows);
  for (int i = 0; i < x.rows; i++) {
    for (int j = 0; j < x.cols; j++) {
      if (j < col) {
        xnull(i, j) = x(i, j);
      } else if (j == col) {
        xcol[i] = x(i, j);
      } else {
        xnull(i, j - 1) = x(i, j);
      }
    }
  }
}

double LinearRegressionScoreTest::GetSEBeta(int idx) const {
  // U = X'Y
  // V = X'X * sigma2
  // beta = X'Y / X'X
  // Var(beta) = Var(U) / (X'X)^2 = V / (V / sigma2)^2 = sigma2^2 / V
  // SE(beta) = sigma2 / sqrt(V)
  const double v = this->Vmatrix(idx, idx);
  if (v == 0.0) {
    return 0.0;
  }
  return (this->GetSigma2() / sqrt(v));
}
